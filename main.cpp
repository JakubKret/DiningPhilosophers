#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <map>

const int NUM_PHILOSOPHERS = 5;

class Fork {
public:
    int id;
    int ownerID;
    bool isClean;
    bool requestToken;

    std::mutex mtx;
    std::condition_variable cv;

    Fork(int id, int owner) : id(id), ownerID(owner), isClean(false), requestToken(false) {}
};


struct Philosopher {
    int id;
    int meals;
    Fork* leftFork;
    Fork* rightFork;
    std::thread t;

    bool hasFork(Fork* f) {
        return f->ownerID == id;
    }
};

std::vector<Philosopher> philosophers(NUM_PHILOSOPHERS);
std::vector<Fork*> forks(NUM_PHILOSOPHERS);


// void run_philosopher(int myID) {
//     Philosopher& me = philosophers[myID];
//
//     while (true) {
//
//         std::cout << "[P" << myID << "] Mysli...\n";
//         std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 500));
//
//         std::cout << "[P" << myID << "] Zglodnial. Probuje zdobyc widelce.\n";
//
//         while (true) {
//             bool hasLeft = me.hasFork(me.leftFork);
//             bool hasRight = me.hasFork(me.rightFork);
//
//             if (hasLeft && hasRight) {
//                 break;
//             }
//             if (!hasLeft) {
//                 std::unique_lock<std::mutex> lock(me.leftFork->mtx);
//                 if (me.leftFork->ownerID != myID) {
//                     me.leftFork->requestToken = true;
//                     me.leftFork->cv.notify_all();
//                 }
//                 me.leftFork->cv.wait(lock, [&] { return me.leftFork->ownerID == myID; });
//             }
//
//             if (!hasRight) {
//                 std::unique_lock<std::mutex> lock(me.rightFork->mtx);
//                 if (me.rightFork->ownerID != myID) {
//                     me.rightFork->requestToken = true;
//                     me.rightFork->cv.notify_all();
//                 }
//                 me.rightFork->cv.wait(lock, [&] { return me.rightFork->ownerID == myID; });
//             }
//         }
//
//         std::cout << "[P" << myID << "] *** JEM ***\n";
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//
//         {
//             std::lock_guard<std::mutex> l1(me.leftFork->mtx);
//             me.leftFork->isClean = false;
//         }
//         {
//             std::lock_guard<std::mutex> l2(me.rightFork->mtx);
//             me.rightFork->isClean = false;
//         }
//         {
//             std::unique_lock<std::mutex> lock(me.leftFork->mtx);
//             if (me.leftFork->requestToken) {
//                 me.leftFork->isClean = true;
//                 me.leftFork->requestToken = false;
//                 me.leftFork->ownerID = (myID == 0 ? NUM_PHILOSOPHERS - 1 : myID - 1);
//                 std::cout << "[P" << myID << "] Oddaje lewy widelec (umyty).\n";
//                 me.leftFork->cv.notify_all();
//             }
//         }
//
//         {
//             std::unique_lock<std::mutex> lock(me.rightFork->mtx);
//             if (me.rightFork->requestToken) {
//                 me.rightFork->isClean = true;
//                 me.rightFork->requestToken = false;
//                 me.rightFork->ownerID = (myID + 1) % NUM_PHILOSOPHERS;
//                 std::cout << "[P" << myID << "] Oddaje prawy widelec (umyty).\n";
//                 me.rightFork->cv.notify_all();
//             }
//         }
//     }
// }


void run_philosopher_rude(int myID) {
    Philosopher& me = philosophers[myID];

    auto acquire_fork = [&](Fork* f, int neighborID) {
        std::unique_lock<std::mutex> lock(f->mtx);

        while (f->ownerID != myID) {
            f->requestToken = true;

            if (!f->isClean) {
                f->isClean = true;
                f->ownerID = myID;
                f->requestToken = false;
                std::cout << "[P" << myID << "] Przejmuje brudny widelec od P" << neighborID << ".\n";
            } else {
                f->cv.wait(lock);
            }
        }
    };

    while (true) {
        std::cout << "[P" << myID << "] Mysli...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 1000));
        std::cout << "[P" << myID << "] Chce jesc...\n";

        int leftNeighbor = (myID == 0 ? NUM_PHILOSOPHERS - 1 : myID - 1);
        int rightNeighbor = (myID + 1) % NUM_PHILOSOPHERS;

        acquire_fork(me.leftFork, leftNeighbor);
        acquire_fork(me.rightFork, rightNeighbor);
        me.meals++;
        std::cout << "[P" << myID << "] Je... " << me.meals << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 500));

        {
            std::lock_guard<std::mutex> l1(me.leftFork->mtx);
            me.leftFork->isClean = false;
            me.leftFork->cv.notify_all();
        }
        {
            std::lock_guard<std::mutex> l2(me.rightFork->mtx);
            me.rightFork->isClean = false;
            me.rightFork->cv.notify_all();
        }
    }
}

int main() {
    srand(time(NULL));

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        int owner = -1;
        int neighbor = (i + 1) % NUM_PHILOSOPHERS;

        if (i < neighbor) owner = i;
        else owner = neighbor;

        forks[i] = new Fork(i, owner);
        forks[i]->isClean = false;
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers[i].id = i;
        philosophers[i].leftFork = forks[i];
        philosophers[i].rightFork = forks[(i + 1) % NUM_PHILOSOPHERS];
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers[i].t = std::thread(run_philosopher_rude, i);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers[i].t.join();
    }

    for(auto f : forks) delete f;

    return 0;
}