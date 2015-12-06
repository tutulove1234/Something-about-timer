/*
 * 	这个文件是用来测试，使用 sigaction 函数和 sigsetjmp函数实现在超时情况下进行一些处理的 。
 *
 * 	在这里，超时使用setitimer 来进行处理的。
 *
 *  程序的执行过程是：
 *  	当没有用户输入时候，定时器会在5秒之后，自动将程序结束，如果在5秒之内有用户键盘输入
 *  并回车，则程序打印输入的内容，并取消定时。
 */

#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>

#define 	TIMED_OUT 			5
#define 	ERROR 				-1

volatile static sig_atomic_t 	can_jmp ;
static sigjmp_buf 	jmpbuf ;

void sig_handler ( int signum ) {
	// 在信号处理函数中，最好不要调用会修改errno值的函数，
	// 或者是不可重入的函数。如果必须调用会修改errno的值
	// 函数，需要事先保存一份当前的errno值在信号处理函数
	// 最后进行恢复。
	
	// 捕捉到 SIGALRM 信号， 进行跳转到对应位置处理。
	siglongjmp ( jmpbuf , 1 ) ;
}

int main () {

	struct itimerval 	tv ;

	bzero (&tv , sizeof tv) ;		

	// 初始化定时器定时时间大小
	tv.it_value.tv_sec = TIMED_OUT ;
	tv.it_value.tv_usec = 0 ;

	struct sigaction 	act ;

	sigemptyset (&act.sa_mask) ;
	act.sa_flags = 0 ;
	act.sa_handler = sig_handler ;

	// 注册信号处理函数
	
	if ( 0 > sigaction ( SIGALRM , &act , NULL ) ) {
		fprintf (stderr , "sigaction error "), perror ("") ;
		exit (1) ;
	}

	// 在这里设置定时器，开始等待超时
	if ( 0 >  setitimer ( ITIMER_REAL , &tv , NULL ) ) {
		fprintf (stderr , "setitimer error ") , perror ("") ;
		exit (1) ;
	}

	if ( 1 == sigsetjmp ( jmpbuf , 1) ) {
		fprintf (stderr , "Wait timed out ! Client quit now !\n") ;	
		/* 这里就可以写你对应关闭socket套接字以及相关的清理处理，，比如：
		 * close (fd) ;
		 * ...
		 */
		return ERROR ;
	} else {
		fprintf ("Meet error ! Can not jmp to a correct position !\n") ;
		abort() ;
	}

	// 在这里用 read() 模拟你的recv() 等使程序阻塞的函数。

	char buf[1024] = {0} ;
	
	bzero (buf , sizeof buf) ;

	int bytes = read ( STDIN_FILENO , buf , sizeof buf ) ;

	tv.it_value.tv_sec = 0 ;
	tv.it_value.tv_usec = 0 ;

	// 取消定时
	if ( 0 > setitimer ( ITIMER_REAL , &tv , NULL ) ) {
		fprintf (stderr , "setitimer error ") , perror ("") ;
		exit (1) ;
	}

	write ( STDOUT_FILENO , buf , strlen (buf)) ;

	return 0 ;
}

