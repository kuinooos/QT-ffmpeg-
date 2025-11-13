#ifndef THREAD_H
#define THREAD_H
#include <thread>

class Thread
{
public:
    Thread(){

    };
    ~Thread(){
        if(thread_){
            thread_->join();
            delete thread_;
            thread_ = NULL;
        }
    }
    int start();
    int stop(){
        abort_ = 1;
        if(thread_){
           thread_->join();
           delete thread_;
           thread_ = NULL;
        }
        return 0;
    };
    virtual void run() = 0;
protected:
    int abort_ = 0;//通知线程是否继续的标志
    std::thread *thread_ = NULL;
};

#endif // THREAD_H
