/*
 * @Author: wiley
 * @LastEditors: wiley
 * @Date: 2020-06-04 16:08:50
 * @LastEditTime: 2020-06-04 16:53:17
 * @Description: file content
 * @FilePath: \TinyWebServer\timer\time_wheel_timer.h
 */ 
#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include "../http/http_conn.h"
#include "../log/log.h"

#define BUFFER_SIZE 64

struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    time_wheel_timer *timer;
};

class time_wheel_timer
{
public:
    int rotation; // 记录定时器在时间轮转多少圈后失效
    int time_slot; // 记录定时器属于时间轮上的哪个槽
    void (*cb_func)(client_data*); // 定时器回调函数
    client_data *user_data; //客户数据
    time_wheel_timer *next;
    time_wheel_timer *prev;

public:
    time_wheel_timer(int rot, int ts) : next(NULL), prev(NULL), rotation(rot), time_slot(ts) {}
};

class time_wheel
{
private:
    /* data */
    static const int N = 60; // 时间轮上的槽的数目
    static const int SI = 1; // 每1s时间轮轮动一次，槽间隔为1s
    time_wheel_timer *slots[N]; // 时间轮的槽，其中每个元素指向一个定时器链表，链表无序
    int cur_slot; // 时间轮的当前槽
    
public:
    time_wheel() : cur_slot(0) {
        for (int i = 0; i < N; i++) {
            slots[i] = NULL;
        }
    }

    ~time_wheel() {
        for (int i = 0; i < N; i++) {
            time_wheel_timer *tmp = slots[i];
            while (tmp) {
                slots[i] = tmp->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }    

    // 根据定时值timeout创建一个定时器，并把它插入合适槽中
    time_wheel_timer *add_timer(int timeout) {
        if (timeout < 0) {
            return NULL;
        }
        int ticks = 0;
        /**
         * 下面根据待插入定时器的超时值计算它将在时间轮转动多少个滴答后被触发，
         * 并将该滴答数存在变量ticks中，如果待插入定时器的超时值小于时间轮的槽间隔SI
         * 则将ticks向上折合为1， 否则就将ticks向下折合为timeout / SI
        */
        if (timeout < SI) {
            ticks = 1;
        }
        else {
            ticks = timeout / SI;
        }
        // 计算待插入的定时器在时间轮转动多少圈后被触发
        int rotation = ticks / N;
        // 计算待插入的定时器应该被插入哪个槽中
        int ts = (cur_slot + (ticks % N)) % N;
        // 创建新的定时器，它在时间轮转动rotation圈之后被触发，且位于第ts个槽上
        time_wheel_timer *timer = new time_wheel_timer(rotation, ts);
        // 如果第ts个槽中尚无任何定时器，则把新建的定时器插入其中，并将该定时器设置为该槽的头结点
        if (!slots[ts]) {
            slots[ts] = timer;
        }
        else {
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }

    // 删除定时器
    void del_timer(time_wheel_timer *timer) {
        if (!timer) {
            return;
        }
        int ts = timer->time_slot;
        // slots[ts] 是目标定时器所在槽的头结点，如果目标定时器就是头结点，则需要重置第ts个槽的头结点
        if (timer == slots[ts]) {
            slots[ts] = slots[ts]->next;
            if (slots[ts]) {
                slots[ts]->prev = NULL;
            }
            delete timer;
        }
        else {
            timer->prev->next = timer->next;
            if (timer->next) {
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
    }

    // SI时间到后，调用该函数，时间轮向前滚动一个槽的间隔
    void tick() {
        time_wheel_timer *tmp = slots[cur_slot]; // 取得当前槽上头结点
        while (tmp)
        {
            // 如果定时器的rotation值大于0，则在这一轮不起作用
            if (tmp->rotation > 0) {
                tmp->rotation--;
                tmp = tmp->next;
            }   
            //否则说明定时器已经到期，于是执行定时任务，然后删除该定时器
            else {
                tmp->cb_func(tmp->user_data);
                if (tmp == slots[cur_slot]) {
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if (slots[cur_slot]) {
                        slots[cur_slot]->prev = NULL;
                    }
                    tmp = slots[cur_slot];
                }
                else {
                    tmp->prev->next = tmp->next;
                    if (tmp->next) {
                        tmp->next->prev = tmp->prev;
                    }
                    time_wheel_timer *tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        cur_slot = ++cur_slot % N;
    }
};

#endif