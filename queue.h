#ifndef QUEUE_H
#define QUEUE_H

#include <mutex>
#include <queue>
#include <condition_variable>

template <typename T>
class Queue
{
public:
    Queue(){};
    ~Queue(){};

    void abort(){
        abort_ = 1;
        cond_.notify_all();
    }

    int push(T val){
        std::lock_guard<std::mutex> lock(mutex_);
        if(abort_ == 1){
            return -1;
        }
        queue_.push(val);
        cond_.notify_one();

        return 0;
    }

    int pop(T &val,const int timeout = 0){
        std::unique_lock<std::mutex> lock(mutex_);//要在等待时(cond_.wait_for)临时解锁，
        //所以用unique_lock

        if(queue_.empty()){
            //等待push或者唤醒（超时）
            cond_.wait_for(lock,std::chrono::milliseconds(timeout),[this]{
                return !queue_.empty() || abort_;
                //如果 返回 true → 条件满足，wait_for 立刻返回（等待结束）。
                //如果 返回 false → 条件还不满足，wait_for 会继续等（直到被唤醒或超时）。
            });
        }//等待timeout，如果有数据就return

        if(abort_ == 1){
            return -1;
        }
        if(queue_.empty()){
            return -2;
        }

        val = queue_.front();
        queue_.pop();
        return 0;
    }

    int front(T &val){
        std::lock_guard<std::mutex> lock(mutex_);
        if(abort_ == 1){
            return -1;
        }
        if(queue_.empty()){
            return -2;
        }

        val = queue_.front();

        return 0;
    }

    int size(){
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }


private:
    int abort_ = 0;
    std::mutex mutex_; //锁
    std::condition_variable cond_; //通知其他线程
    std::queue<T> queue_;
};

#endif // QUEUE_H
