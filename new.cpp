#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <chrono>
#include <random>

using namespace std;

mutex mutex1;

// Initializing number of threads for producer and consumer
int producer_threads = 4;
int consumer_threads = 4;

int hr_indicator = 48; // This value will be used to check if an hour has passed (48 rows for an hour)

int prod_count = 0; // Producer counter
int cons_count = 0; // consumer counter
int rows = 0;       // Number of rows in the dataset

condition_variable prod_cv, cons_cv; // Initializing condition variables for producer and consumer

// String variables and vectors are initialized to get values from the data file
string ind, t_stamp, tr_light_id, no_of_cars;
vector<int> in;
vector<int> tr_light;
vector<int> no_cars;
vector<string> tstamp;

// Struct for traffic data row
struct Traffic_Signal
{
    int ind;
    std::string t_stamp;
    int tr_id;
    int num_cars;
};

// Traffic_Signal array of four is initialized to hold the totals of each of the 4 traffic lights
Traffic_Signal tlSorter[4] = {{0, "", 1, 0}, {0, "", 2, 0}, {0, "", 3, 0}, {0, "", 4, 0}};

// Queue to store traffic light data
queue<Traffic_Signal> tr_sig_queue;
Traffic_Signal sig;

// Function to sort traffic light data
bool sortmethod(struct Traffic_Signal first, struct Traffic_Signal second)
{
    return first.num_cars > second.num_cars;
}

// Function to sort traffic light data
void produce()
{
    while (prod_count < rows)
    {
        unique_lock<mutex> lk(mutex1); // locking until producer finishes processing

        if (prod_count < rows) // if count is less than the number of rows in the dataset
        {
            tr_sig_queue.push(Traffic_Signal{in[prod_count], tstamp[prod_count], tr_light[prod_count], no_cars[prod_count]}); // push into queue
            cons_cv.notify_all();                                                                                             // notifying consumer threads
            prod_count++;
        }
        else
        {
            prod_cv.wait(lk, []
                         { return prod_count < rows; }); // if count is greater than the number of rows in the data set wait
        }

        lk.unlock();                                                 // unlock after processing
        this_thread::sleep_for(chrono::milliseconds(rand() % 3000)); // Simulating some processing time
    }
}

// Consumer function to pop data from the queue
void consume()
{
    while (cons_count < rows)
    {
        unique_lock<mutex> lk(mutex1); // lock until processing

        if (!tr_sig_queue.empty())
        {
            sig = tr_sig_queue.front(); // getting the front elements of the queue

            // add the number of cars into the respective traffic light id
            if (sig.tr_id >= 1 && sig.tr_id <= 4)
            {
                tlSorter[sig.tr_id - 1].num_cars += sig.num_cars;
            }

            tr_sig_queue.pop();   // pop the data
            prod_cv.notify_all(); // notify producer
            cons_count++;
        }
        else
        {
            cons_cv.wait(lk, []
                         { return !tr_sig_queue.empty(); }); // if queue wait until producer produces
        }

        // check if an hour passed by, checking every 48th row
        if (cons_count % hr_indicator == 0)
        {
            sort(tlSorter, tlSorter + 4, sortmethod); // sorting data
            printf("Traffic lights sorted according to most busy| Time: %s \n", sig.t_stamp.c_str());
            cout << "Traffic Light"
                 << "\t"
                 << "Number of Cars" << endl;
            for (int i = 0; i < 4; ++i)
            {
                cout << tlSorter[i].tr_id << "\t"
                     << "\t"
                     << tlSorter[i].num_cars << endl;
            }
        }

        lk.unlock();
        this_thread::sleep_for(chrono::milliseconds(rand() % 3000));
    }
}

// function to get data from file
void get_traff_data()
{
    ifstream infile;

    string file;
    cout << "Enter the filename: ";
    cin >> file;

    infile.open(file);

    if (infile.is_open())
    {
        std::string line;
        getline(infile, line);

        while (!infile.eof())
        {
            getline(infile, ind, ',');
            in.push_back(stoi(ind));
            getline(infile, t_stamp, ',');
            tstamp.push_back(t_stamp);
            getline(infile, tr_light_id, ',');
            tr_light.push_back(stoi(tr_light_id));
            getline(infile, no_of_cars, '\n');
            no_cars.push_back(stoi(no_of_cars));

            rows += 1;
        }
        infile.close();
    }
    else
    {
        printf("Can not open the file, TRY AGAIN!!");
    }
}

int main()
{
    get_traff_data();

    vector<thread> producers(producer_threads);
    vector<thread> consumers(consumer_threads);

    for (int i = 0; i < producer_threads; i++)
        producers[i] = thread(produce);
    for (int i = 0; i < consumer_threads; i++)
        consumers[i] = thread(consume);

    for (int i = 0; i < producer_threads; i++)
        producers[i].join();
    for (int i = 0; i < consumer_threads; i++)
        consumers[i].join();
}