/**
 * 半同步/半反应堆线程池：主线程监听listen socket和接收到的所有连接socket，当有客户端请求任务时，
 * 将任务对象插入到工作任务对象中；等待在任务队列上的工作线程通过竞争来取得任务对象并处理之。
 * 其中的工作任务队列完成了主线程与工作线程之间的解耦，
 * 但是由于同一客户连接的任务请求可能由不同的线程来处理，所以这要求所有的客户请求是无状态的。
*/

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request);

private:
    //注意： 当把线程函数封装在类中，this指针会作为默认的参数被传进函数中，从而和线程函数参数(void*)不能匹配。
    //解决： 线程函数作为静态函数，因为在C++中静态函数没有this指针
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *worker(void *arg);
    void run();

private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    bool m_stop;                //是否结束线程
    connection_pool *m_connPool;  //数据库
};
template <typename T>
threadpool<T>::threadpool( connection_pool *connPool, int thread_number, int max_requests) : m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(NULL),m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
        throw std::exception();
    for (int i = 0; i < thread_number; ++i)
    {
        //printf("create the %dth thread\n",i);
        //创建线程（注意线程函数为静态成员函数，且将this指针作为参数，以访问进程池的成员变量）
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        //将工作线程设置为脱离线程（当脱离线程退出时，系统会自动回收该线程的资源），否则需要其他线程来回收该线程的资源
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
    m_stop = true;
}
template <typename T>
bool threadpool<T>::append(T *request)
{
    m_queuelocker.lock();//由于任务队列被所有线程共享，因此需要先加锁
    if (m_workqueue.size() > m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();//将信号量+1，如果大于0，则唤醒等待该信号量的工作线程
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

//工作线程的处理函数：从任务队列中取出任务对象，然后调用任务对象的逻辑处理函数完成客户请求任务
template <typename T>
void threadpool<T>::run() 
{
    while (!m_stop)
    {
        m_queuestat.wait();//等待信号量大于0
        m_queuelocker.lock();//等待信号量大于0
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_workqueue.front();//取出任务对象
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request)
            continue;
        request->mysql = m_connPool->GetConnection();
        request->process();//处理客户任务
        m_connPool->ReleaseConnection(request->mysql);
    }
}
#endif
