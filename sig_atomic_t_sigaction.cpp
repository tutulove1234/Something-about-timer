/*
   这个文件是用来实现通过原子操作，和信号处理函数实现当出现一定信号时候，对程序进行相关清理操作。
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>

#define TIMED_OUT 5
#define BUFFER_SIZE 1024

// 这里是一个全局的标志， 采用原子变量
static volatile sig_atomic_t timedout_flag = 0;

// 原子的将 timedout_flag 置 1
void sig_handler(int signum) { __sync_fetch_and_add(&timedout_flag, 1); }

int main() {
	struct sigaction old_act, new_act;

	new_act.sa_handler = sig_handler ;
	new_act.sa_flags = 0 ;
	sigemptyset (&new_act.sa_mask) ;

	// 注册信号处理函数， 对应定时器到时时调用sig_handler
	if (0 > sigaction(SIGALRM, &new_act, &old_act)) {
		fprintf(stderr, "sigaction error "), perror("");
		exit(1);
	}

	struct itimerval new_timer ;

	bzero (&new_timer , sizeof new_timer ) ;

	new_timer.it_value.tv_sec = TIMED_OUT;
	new_timer.it_value.tv_usec = 0;

	// 在这里启动定时器，开始等待是否超时。

	if ( 0 > setitimer (ITIMER_REAL , (const struct itimerval *)&new_timer , NULL ) ) {
		fprintf(stderr, "setitimer error , line %d\n" , __LINE__) ;
		exit(1);
	}

	// 在这里用read 来模拟任何阻塞的系统调用

	char buffer[BUFFER_SIZE] = {0};
	int bytes = read(STDIN_FILENO, buffer, BUFFER_SIZE);

	if ( 0 < bytes ) {
		printf("正常收到键盘输入。\n");
		// 设置全局标志为 0
		__sync_lock_test_and_set(&timedout_flag, 0);
	}

	// 判断是否产生超时事件 。
	if (1 == timedout_flag) {
		printf("等待超时!\n");
		// 在这里做一些清理操作，比如 close (fd) 等， free() 空间等。

		printf("Quit programm now .\n");
	} else {
		// 未发生超时事件，即正常输入了数据。
		// 取消闹钟
		new_timer.it_value.tv_usec = 0 ;
		if (0 > setitimer(ITIMER_REAL, &new_timer, NULL)) {
			fprintf(stderr, "setitmer error "), perror("");
			exit(1);
		}
		write(STDOUT_FILENO, buffer, strlen(buffer));
	}

	return 0;
}
