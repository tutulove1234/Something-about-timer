/*
	这个文件用来使用timerfd_create来进行创建定时事件，
	通过epoll来进行管理。超时时进行处理超时事件。
	按照耗子的说法，应当设计并封装一个类，让此类事件通过单独的线程处理。
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>

#define 	ERROR 					-1
#define 	OK 						1

#define 	TIMED_OUT				5
#define 	MAX_EVENTS 				256
#define 	BUFFER_SIZE 			1024

// 添加新的需要监听的文件描述符到epoll
int epoll_add (int epollfd , int addfd ) {

	if ( 0 > epollfd || 0 > addfd ) {
		fprintf (stderr , "Pass in bad arguments!\n") ;
		return ERROR ;
	}

	struct 	epoll_event 	ep_event ;
	bzero ( &ep_event , sizeof ep_event ) ;

	ep_event.data.fd = addfd ;
	ep_event.events = EPOLLIN ;

	if ( 0 > epoll_ctl (epollfd , EPOLL_CTL_ADD , addfd , &ep_event)) {
		fprintf (stderr , "epoll_ctl error ") , perror ("") ;
		return ERROR ;
	}

	return OK ;
}

int epoll_remove ( int epollfd , int removefd ) {

	if ( 0 > epollfd || 0 > removefd ) {
		fprintf (stderr , "Pass in bad arguments!\n") ;
		return ERROR ;
	}

	if ( 0 > epoll_ctl (epollfd , EPOLL_CTL_DEL , removefd , NULL )) {
		fprintf (stderr , "epoll_ctl error ") , perror ("") ;
		return ERROR ;
	}

	return OK ;
}

// 简单写以下， 通过epoll来监听用户输入事件和定时器事件。当在指定事件内没有完成输入，就默认
// 表示超时，超时的处理有用户来进行。

int main () {

// 创建一个定时器事件
	int timerfd = timerfd_create( CLOCK_REALTIME , 0) ;
	int epollfd = epoll_create (1) ;

// 创建定时器
	struct itimerspec new_timer , old_timer ;

	bzero (&new_timer , sizeof new_timer) ;
	new_timer.it_value.tv_sec  = TIMED_OUT ;
	new_timer.it_value.tv_nsec = 0 ;

	if ( ERROR == epoll_add ( epollfd , timerfd) ) {
		exit (1) ;
	}
	if ( ERROR == epoll_add ( epollfd , STDIN_FILENO )) {
		exit (1) ;
	}

// 设置定时器
	if ( 0 > timerfd_settime( timerfd , 0 , (const struct itimerspec*)&new_timer , &old_timer)) {
		fprintf (stderr , "timerfd_settime error ") , perror ("") ;
		exit (1) ;
	}

	struct 	epoll_event 	events[MAX_EVENTS] ;
	char 	input_buffer[BUFFER_SIZE] = {0} ;

	while (1) {
		/// 在这里也可以直接用 epoll_wait 自带的定时器。通过处理epoll的返回值就可以。
		int events_count = epoll_wait ( epollfd , events , MAX_EVENTS , -1) ;

		for ( int i = 0 ; i < events_count ; i++ ) {
			if ( events[i].data.fd == STDIN_FILENO) {
				int ret = read ( STDIN_FILENO , input_buffer , BUFFER_SIZE) ;
				printf ("Get input info , cancle timer now!\n") ;
				write ( STDOUT_FILENO , input_buffer , ret) ;
				// 获得正常输入，取消定时器 。在这里的取消定时器直接将timerfd 从epoll的监听集合里去除就可以。
				if ( ERROR == epoll_remove ( epollfd , events[i].data.fd)) {
					exit (1) ;
				}
			} else if ( events[i].data.fd == timerfd ) {
				printf ("Waited timed out !\nQuit now!\n") ;
				//在这里做一些清理工作， 比如 close (fd); 等操作 。
				exit (1) ;
			}
		}
	}
	return 0 ;
}
