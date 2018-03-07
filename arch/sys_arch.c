/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
/*  Porting by Michael Vysotsky <michaelvy@hotmail.com> August 2011   */

#define SYS_ARCH_GLOBALS

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/lwip_sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "FreeRTOS.h"
#include "task.h"
#include "includes.h"
#include "delay.h"
#include "arch/sys_arch.h"
#include "malloc.h"

xTaskHandle xTaskGetCurrentTaskHandle( void ) PRIVILEGED_FUNCTION;

/* This is the number of threads that can be started with sys_thread_new() */
#define SYS_THREAD_MAX 6

static u16_t s_nextthread = 0;

//����һ����Ϣ����
//*mbox:��Ϣ����
//size:�����С
//����ֵ:ERR_OK,�����ɹ�
//         ����,����ʧ��
err_t sys_mbox_new( sys_mbox_t *mbox, int size)
{
    (void ) size;
    //archMESG_QUEUE_LENGTH��Ϣ���г�����sys_arch.�ж���
    *mbox = xQueueCreate( archMESG_QUEUE_LENGTH, sizeof( void * ) );//����һ����Ϣ����
#if SYS_STATS
    ++lwip_stats.sys.mbox.used;
    if (lwip_stats.sys.mbox.max < lwip_stats.sys.mbox.used)
    {
        lwip_stats.sys.mbox.max = lwip_stats.sys.mbox.used;
    }
#endif /* SYS_STATS */
    if (*mbox == NULL) //����ʧ��
        return ERR_MEM;

    return ERR_OK;
}

//�ͷŲ�ɾ��һ����Ϣ����
//*mbox:Ҫɾ������Ϣ����
/*
  Deallocates a mailbox. If there are messages still present in the
  mailbox when the mailbox is deallocated, it is an indication of a
  programming error in lwIP and the developer should be notified.
*/
void sys_mbox_free(sys_mbox_t * mbox)
{
    if( uxQueueMessagesWaiting( *mbox ) )
    {
        /* Line for breakpoint.  Should never break here! */
        portNOP();
#if SYS_STATS
        lwip_stats.sys.mbox.err++;
#endif /* SYS_STATS */
        // TODO notify the user of failure.
    }
    vQueueDelete( *mbox );
#if SYS_STATS
    --lwip_stats.sys.mbox.used;
#endif /* SYS_STATS */
}
//����Ϣ�����з���һ����Ϣ(���뷢�ͳɹ�)
//*mbox:��Ϣ����
//*msg:Ҫ���͵���Ϣ
void sys_mbox_post(sys_mbox_t *mbox,void *msg)
{    
    //��ѭ���ȴ���Ϣ���ͳɹ�
    while ( xQueueSendToBack(*mbox, &msg, portMAX_DELAY ) != pdTRUE ) {}
}
//������һ����Ϣ���䷢����Ϣ
//�˺��������sys_mbox_post����ֻ����һ����Ϣ��
//����ʧ�ܺ󲻻᳢�Եڶ��η���
//*mbox:��Ϣ����
//*msg:Ҫ���͵���Ϣ
//����ֵ:ERR_OK,����OK
// 	     ERR_MEM,����ʧ��
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{ 
    err_t result;
    if ( xQueueSend( *mbox, &msg, 0 ) == pdPASS )
    {
        result = ERR_OK;//���ͳɹ�
    }
    else
    {
        // could not post, queue must be full
        result = ERR_MEM; //����ʧ��
#if SYS_STATS
        lwip_stats.sys.mbox.err++;
#endif /* SYS_STATS */

    }
    return result;
}

//�ȴ������е���Ϣ
//*mbox:��Ϣ����
//*msg:��Ϣ
//timeout:��ʱʱ�䣬���timeoutΪ0�Ļ�,��һֱ�ȴ�
//����ֵ:��timeout��Ϊ0ʱ����ɹ��Ļ��ͷ��صȴ���ʱ�䣬
//		ʧ�ܵĻ��ͷ��س�ʱSYS_ARCH_TIMEOUT
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{ 
    void *dummyptr;
    portTickType StartTime, EndTime, Elapsed;

    StartTime = xTaskGetTickCount();

    if ( msg == NULL )//��ϢΪ��
    {
        msg = &dummyptr;//ָ���ָ��
    }

    if ( timeout != 0 )
    {
        if ( pdTRUE == xQueueReceive( *mbox, &(*msg), timeout / portTICK_RATE_MS ) ) //��ʱ���ڻ����Ϣ
        {
            EndTime = xTaskGetTickCount();//��ȡ����
            Elapsed = (EndTime - StartTime) * portTICK_RATE_MS; //���ʱ��

            return ( Elapsed ); //����ʱ��
        }
        else // ��ʱδ���ܵ���Ϣ���������ֵ
        {
            *msg = NULL;

            return SYS_ARCH_TIMEOUT;
        }
    }
    else // timeout Ϊ 0 ��ʾ��Զ�ȴ���Ϣ���ճɹ�
    {
        while( pdTRUE != xQueueReceive( *mbox, &(*msg), portMAX_DELAY ) ) {} //ʱ�����⣬ֱ����ȡ����Ϣ
        EndTime = xTaskGetTickCount();
        Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

        return ( Elapsed ); // return time blocked TODO test
    }
}
//���Ի�ȡ��Ϣ
//*mbox:��Ϣ����
//*msg:��Ϣ
//����ֵ:�ȴ���Ϣ���õ�ʱ��/SYS_ARCH_TIMEOUT
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	void *dummyptr;
	if ( msg == NULL )
	{
		msg = &dummyptr;
	}
	if ( pdTRUE == xQueueReceive( *mbox, &(*msg), 0 ) ) //���ȴ���ȡʱ��
	{
		return ERR_OK;
	}
	else
	{
		return SYS_MBOX_EMPTY;//��ϢΪ�գ�δ��ȡ����Ϣ
	}
}
//���һ����Ϣ�����Ƿ���Ч
//*mbox:��Ϣ����
//����ֵ:1,��Ч.
//      0,��Ч
int sys_mbox_valid(sys_mbox_t *mbox)
{  
  if (*mbox == SYS_MBOX_NULL) 
    return 0;
  else
    return 1;
} 
//����һ����Ϣ����Ϊ��Ч
//*mbox:��Ϣ����
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	*mbox = SYS_MBOX_NULL;
} 
//����һ���ź���
//*sem:�������ź���
//count:�ź���ֵ
//����ֵ:ERR_OK,����OK
// 	     ERR_MEM,����ʧ��
err_t sys_sem_new(sys_sem_t * sem, u8_t count)
{  
	vSemaphoreCreateBinary(*sem ); //����һ���ź���(0/1)
	if(*sem == NULL) //����ʧ��
	{
#if SYS_STATS
      ++lwip_stats.sys.sem.err;
#endif /* SYS_STATS */	
		return ERR_MEM;
	}
	if(count == 0)	// Means it can't be taken
	{
		xSemaphoreTake(*sem,1);
	}
#if SYS_STATS
	++lwip_stats.sys.sem.used;
 	if (lwip_stats.sys.sem.max < lwip_stats.sys.sem.used) {
		lwip_stats.sys.sem.max = lwip_stats.sys.sem.used;
	}
#endif /* SYS_STATS */
		
	return ERR_OK;;
} 
//�ȴ�һ���ź���
//*sem:Ҫ�ȴ����ź���
//timeout:��ʱʱ��
//����ֵ:��timeout��Ϊ0ʱ����ɹ��Ļ��ͷ��صȴ���ʱ�䣬
//		ʧ�ܵĻ��ͷ��س�ʱSYS_ARCH_TIMEOUT
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{ 
	portTickType StartTime, EndTime, Elapsed;
	StartTime = xTaskGetTickCount();//��ȡ��ǰ������
	if(	timeout != 0)
	{
		if( xSemaphoreTake( *sem, timeout / portTICK_RATE_MS ) == pdTRUE )//�ɹ���ȡ���ź�
		{
			EndTime = xTaskGetTickCount();
			Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;
			return (Elapsed); // return time blocked TODO test	
		}
		else //��ʱ
		{
			return SYS_ARCH_TIMEOUT;
		}
	}
	else // ֱ����ȡ�ɹ�
	{
		while( xSemaphoreTake(*sem, portMAX_DELAY) != pdTRUE){}
		EndTime = xTaskGetTickCount();
		Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

		return ( Elapsed ); // return time blocked	
		
	}
}
//����һ���ź���
//sem:�ź���ָ��
void sys_sem_signal(sys_sem_t *sem)
{
	xSemaphoreGive(*sem);
}
//�ͷŲ�ɾ��һ���ź���
//sem:�ź���ָ��
void sys_sem_free(sys_sem_t *sem)
{
#if SYS_STATS
      --lwip_stats.sys.sem.used;
#endif /* SYS_STATS */
			
	vQueueDelete(*sem);
} 
//��ѯһ���ź�����״̬,��Ч����Ч
//sem:�ź���ָ��
//����ֵ:1,��Ч.
//      0,��Ч
int sys_sem_valid(sys_sem_t *sem)
{
  if (*sem == SYS_SEM_NULL)
    return 0;
  else
    return 1;           
} 
//����һ���ź�����Ч
//sem:�ź���ָ��
void sys_sem_set_invalid(sys_sem_t *sem)
{
	*sem=SYS_SEM_NULL;
} 
//arch��ʼ��
void sys_init(void)
{ 
    //����,�����ڸú���,�����κ�����
	// keep track of how many threads have been created
	s_nextthread = 0;
} 

/*-----------------------------------------------------------------------------------*/
                                      /*��������ʹ��*/
/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
#if LWIP_COMPAT_MUTEX == 0 //Ϊ0ʹ�û����ź�
/* Create a new mutex*/
//����һ���źŻ�����
err_t sys_mutex_new(sys_mutex_t *mutex) {

  *mutex = xSemaphoreCreateMutex();
		if(*mutex == NULL)
	{
#if SYS_STATS
      ++lwip_stats.sys.mutex.err;
#endif /* SYS_STATS */	
		return ERR_MEM;
	}

#if SYS_STATS
	++lwip_stats.sys.mutex.used;
 	if (lwip_stats.sys.mutex.max < lwip_stats.sys.mutex.used) {
		lwip_stats.sys.mutex.max = lwip_stats.sys.mutex.used;
	}
#endif /* SYS_STATS */
        return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/* Deallocate a mutex*/
//ɾ��һ��������
void sys_mutex_free(sys_mutex_t *mutex)
{
#if SYS_STATS
      --lwip_stats.sys.mutex.used;
#endif /* SYS_STATS */
			
	vQueueDelete(*mutex);
}
/*-----------------------------------------------------------------------------------*/
/* Lock a mutex*/
//����
void sys_mutex_lock(sys_mutex_t *mutex)
{
	sys_arch_sem_wait(*mutex, 0);
}

/*-----------------------------------------------------------------------------------*/
/* Unlock a mutex*/
//����
void sys_mutex_unlock(sys_mutex_t *mutex)
{
	xSemaphoreGive(*mutex);
}
#endif /*LWIP_COMPAT_MUTEX*/

/*-----------------------------------------------------------------------------------*/
// TODO
//�������ϵͳ֧���̲߳���LWIPҲ��Ҫʹ���߳���ô����Ҫʵ������ĺ���
/*-----------------------------------------------------------------------------------*/

//extern OS_STK * TCPIP_THREAD_TASK_STK;//TCP IP�ں������ջ,��lwip_comm��������
//����һ���½���
//*name:��������
//thred:����������
//*arg:�����������Ĳ���
//stacksize:��������Ķ�ջ��С
//prio:������������ȼ�
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	xTaskHandle CreatedTask;
	int result;

   if ( s_nextthread < SYS_THREAD_MAX )
   {
	   //vPortEnterCritical(); //�����ٽ���
      result = xTaskCreate( thread, ( const portCHAR * ) name, stacksize, arg, prio, &CreatedTask );
	  // vPortExitCritical(); //�˳��ٽ���
	   if(result == pdPASS)
		   return CreatedTask;
	   else 
		   return NULL;
   }
   else return NULL;
}
/*
  This optional function does a "fast" set of critical region protection to the
  value specified by pval. See the documentation for sys_arch_protect() for
  more information. This function is only required if your port is supporting
  an operating system.*/
//��Щ����ʵ���ٽ��������Ĺ���
//�ٽ�������
sys_prot_t sys_arch_protect(void)
{	
	//�����ٽ���
	return portSET_INTERRUPT_MASK_FROM_ISR();
}
//�˳��ٽ���
void sys_arch_unprotect(sys_prot_t pval)
{
	portCLEAR_INTERRUPT_MASK_FROM_ISR(pval);  //�˳��ٽ���
}
//ϵͳ����
//��ʱδ�õ�
void sys_assert( const char *msg )
{	
	( void ) msg;
	/*FSL:only needed for debugging
	printf(msg);
	printf("\n\r");
	*/
    portSET_INTERRUPT_MASK_FROM_ISR(  );//�����ٽ���
    for(;;)
    ;
}
//��ȡϵͳʱ��
//����ֵ:��ǰϵͳʱ��(��λ:����)
u32_t sys_now(void)
{
//    u32_t rtos_time, lwip_time;
	return (xTaskGetTickCount());	//��ȡ��ǰ������
//	lwip_time=(rtos_time*1000/configTICK_RATE_HZ+1);//��������ת��ΪLWIP��ʱ��MS
//	return lwip_time; 		//����lwip_time;
}


//�������DNS TXID
u16_t LWIP_RAND()
{
	static uint8_t flag=1;
	if(flag)
	{
		flag=0;
		srand(sys_now()); 
	}
	return rand();
}
////lwip��ʱ����
////ms:Ҫ��ʱ��ms��
//void sys_msleep(u32_t ms)
//{
//	delay_ms(ms);
//}
////��ȡϵͳʱ��,LWIP1.4.1���ӵĺ���
////����ֵ:��ǰϵͳʱ��(��λ:����)
//u32_t sys_now(void)
//{
//	u32_t ucos_time, lwip_time;
//	ucos_time=OSTimeGet();	//��ȡ��ǰϵͳʱ�� �õ�����UCSO�Ľ�����
//	lwip_time=(ucos_time*1000/OS_TICKS_PER_SEC+1);//��������ת��ΪLWIP��ʱ��MS
//	return lwip_time; 		//����lwip_time;
//}













































