/*
 * @Author: wiley
 * @LastEditors: wiley
 * @Date: 2020-06-04 16:57:52
 * @LastEditTime: 2020-06-04 17:35:26
 * @Description: file content
 * @FilePath: \TinyWebServer\timer\min_heap.h
 */ 
#ifndef MIN_HEAP
#define MIN_HEAP

#include <time.h>
#include "../http/http_conn.h"
#include "../log/log.h"

using std::exception;

#define BUFFER_SIZE 64

class heap_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    heap_timer *timer;
};

class heap_timer {
public:
    time_t expire;
    void (*cb_func)(client_data*);
    client_data *user_data;
public:
    heap_timer(int delay) {
        expire = time(NULL) + delay;
    }
};

class time_heap {
private:
    heap_timer **array;
    int capacity;
    int cur_size();
public:
    time_heap(int cap) throw (std::exception) :capacity(cap), cur_size(0) {
        array = new heap_timer *(capacity);
        if (!array) {
            throw std::exception();
        }
        for (int i = 0; i < capacity; i++) {
            array[i] = NULL;
        }
    }

    time_heap(heap_timer **init_array, int size, int capacity) 
    throw(std::exception) : cur_size(size), capacity(capacity) {
        if (capacity < size) {
            throw std::exception();
        }
        array = new heap_timer *(capacity);
        if (!array) {
            throw std::exception();
        }
        for (int i = 0; i < capacity; i++) {
            array[i] = NULL;
        }
        if (size != 0) {
            for (int i = 0; i < size; i++) {
                array[i] = init_array[i];
            }
            for (int i = (cur_size - 1) / 2; i >= 0; --i) {
                percolate_down(i);
            }
        }
    }

    ~time_heap() {
        for (int i = 0; i < cur_size(); i++) {
            delete array[i];
        }
        delete[] array;
    }

public:
    void add_timer(heap_timer* timer) throw(std::exception) {
        if (!timer) {
            return ;
        }
        if (cur_size >= capacity) {
            resize();
        }
        int hole = cur_size++;
        int parent = 0;
        for (; hole > 0; hole = parent) {
            parent = (hole - 1) / 2;
            if (array[parent]->expire <= timer->expire) {
                break;
            }
            array[hole] = array[parent];
        }
        array[hole] = timer;
    }

    void del_timer(heap_time *timer) {
        if (!timer) {
            return;
        }
        timer->cb_func = NULL;
    }

    heap_timer *top() const {
        if (empty()) {
            return NULL;
        }
        return array[0];
    }

    void pop_timer() {
        if (empty()) {
            return;
        }
        if (array[0]) {
            delete array[0];
            array[0] = array[--cur_size];
            percolate_down(0);
        }
    }

    void tick() {
        heap_timer *tmp = array[0];
        time_t cur = time(NULL);
        while(!empty()) {
            if (!tmp) {
                break;
            }
            if(tmp->expire > cur) {
                break;
            }
            if (array[0]->cb_func) {
                array[0]->cb_func(array[0]->user_data);
            }
            pop_timer();
            tmp = array[0];
        }
    }
    bool empty() const {return cur_size == 0;}

private:
    // 最小堆向下操作
    void percolate_down(int hole) {
        heap_timer *temp = array[hole];
        int child = 0;
        for (; ((hole * 2 + 1) <= (cur_size - 1)); hole = child) {
            child = hole * 2 + 1;
            if ((child < (cur_size - 1)) && (array[child+1]->expire < array[child]->expire)) {
                ++child;
            }
            if (array[child->expire] < temp->expire) {
                array[hole] = array[child];
            }
            else {
                break;
            }
        }
        array[hole] = temp; 
    }

    // 堆数组扩容一倍
    void resize() throw (std::exception) {
        heap_timer **temp = new heap_timer * [2 * capacity];
        for (int i = 0; i < 2 * capacity; ++i) {
            temp[i] = NULL;
        }
        if (!temp) {
            throw std::exception();
        }
        capacity = 2 * capacity;
        for (int i = 0; i < cur_size; ++i) {
            temp[i] = array[i];
        }
        delete[] array;
        array = temp;
    }
};

#endif
