/////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015, STEREOLABS.
//
// All rights reserved.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUD0ING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////


/****************************************************************************************************
 ** This sample demonstrates how to grab images and depth map with the ZED SDK in parallel threads **
 ***************************************************************************************************/


//standard includes
#include <stdio.h>
#include <string.h>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>

//opencv includes
#include <opencv2/opencv.hpp>

//ZED Includes
#include <zed/Camera.hpp>
#include <zed/utils/GlobalDefine.hpp>

//Time record
#include <time.h> 

//wring to file
#include <fstream>

using namespace sl::zed;
using namespace std;

// Exchange structure

typedef struct image_bufferStruct {
    unsigned char* data_iml;
    std::mutex mutex_input_imagel;

    unsigned char* data_imr;
    std::mutex mutex_input_imager;

    //float* data_depth;
    //std::mutex mutex_input_depth;

    int width, height, im_channels;
} image_buffer;


Camera* zed;
image_buffer* buffer;

SENSING_MODE dm_type = STANDARD;

bool stop_signal;
int count_run = 0;
bool newFrame = false;

// Grabbing function

void grab_run() {
    
   // float* p_depth;
   uchar* p_left;
   uchar* p_right;
   //Time record
   time_t start_time,end_time;
   ofstream outfile;
   outfile.open("readPicturefile.txt");
	

#ifdef __arm__ //only for Jetson K1/X1
    sl::zed::Camera::sticktoCPUCore(2);
#endif
 start_time = clock();
/*******************************************************************************************/
// 改动点：注释一行，添加三行
// 时间：2018/10/18
// 作者：张旭东
// 说明：这是多线程之间的PV操作，stop_signal是“消费者”完成标志。
/******************************************************************************************/
    // while (!stop_signal) {
		 while(1){
			 
		  while(stop_signal == true) //等待主线程将数据存成文件的过程执行完毕
		  {
			  std::this_thread::sleep_for(std::chrono::milliseconds(1));
		  }
		  stop_signal = true;
/******************************************************************************************/		  
        if (!zed->grab(dm_type, 1, 1)) {   //直接换成STANDARD

            //p_depth = (float*) zed->retrieveMeasure(MEASURE::DEPTH).data; // Get the pointer
            p_left = zed->retrieveImage(SIDE::LEFT).data;                   // Get the pointer
            p_right = zed->retrieveImage(SIDE::RIGHT).data;                 // Get the pointer

            if (count_run % 1000 == 0) {
                std::cout << "* last grab picture TimeStamp : " << zed->getCameraTimestamp()/1000 << "us" << std::endl;
                long long current_ts = zed->getCurrentTimestamp();
                std::cout << "** Current TimeStamp : " << current_ts << std::endl;
					 std::cout << "*** grab the 1000 picture in time: " << (current_ts-zed->getCameraTimestamp())/1000 << "us" << std::endl;
            }

            //Time record
            // time_t start_time,end_time;
            // ofstream outfile;
            // outfile.open("readPicturefile.txt");
            // Fill the buffer
            //buffer->mutex_input_depth.lock(); // To prevent from data corruption
            //memcpy(buffer->data_depth, p_depth, buffer->width * buffer->height * sizeof (float));
           // buffer->mutex_input_depth.unlock();
           

            buffer->mutex_input_imagel.lock();
            memcpy(buffer->data_iml, p_left, buffer->width * buffer->height * buffer->im_channels * sizeof (uchar));
            buffer->mutex_input_imagel.unlock();

            buffer->mutex_input_imager.lock();
            memcpy(buffer->data_imr, p_right, buffer->width * buffer->height * buffer->im_channels * sizeof (uchar));
            buffer->mutex_input_imager.unlock();
            
           
            newFrame = true;                //"生产者"通知"消费者"数据已经生产完毕。
            count_run++;
        }  else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
            end_time = clock();
            cout << "The readsPictureTime is:" << (end_time - start_time)/1000 << "us!" << endl;
            outfile << "readPictureTime:" << (end_time - start_time)/1000 << "us!" <<endl;
            outfile.close();
}

int main(int argc, char **argv)
{
    stop_signal = false;  // "消费者" 通知"生产者"首先产生数据

    if (argc == 1) // Use in Live Mode
        zed = new Camera(HD720, 60);//VGA?HD720
		//zed = new Camera(VGA, 60);//VGA?HD720
		//zed = new Camera(HD2K,15);
    else // Use in SVO playback mode
        zed = new Camera(argv[1]);
    cout<<"new comera successful"<<endl;

		//writing picture
		// time_t start_time,end_time;
		// ofstream outfile;
		// outfile.open("writePicturefile.txt");

    InitParams parameters;
    parameters.mode = PERFORMANCE;
    parameters.unit = MILLIMETER;
    parameters.verbose = 1;

    ERRCODE err = zed->init(parameters);
    cout << errcode2str(err) << endl;
    if (err != SUCCESS) {
        delete zed;
        return 1;
    }

	  int width = zed->getImageSize().width;
	  int height = zed->getImageSize().height;
     
    
    // allocate data
    buffer = new image_buffer();
    buffer->height = height;
    buffer->width = width;
    buffer->im_channels = 4;
    //buffer->data_depth = new float[buffer->height * buffer->width];
    //**************************************************
    buffer->mutex_input_imagel.lock();
    buffer->data_iml = new uchar[buffer->height * buffer->width * buffer->im_channels];
    buffer->mutex_input_imagel.unlock();
    buffer->mutex_input_imager.lock();
    buffer->data_imr = new uchar[buffer->height * buffer->width * buffer->im_channels];
    buffer->mutex_input_imager.unlock();

    cv::Mat left(height, width, CV_8UC4, buffer->data_iml, buffer->width * buffer->im_channels * sizeof (uchar)); //一行中所有元素的字节数量
    cv::Mat right(height, width, CV_8UC4, buffer->data_imr, buffer->width * buffer->im_channels * sizeof (uchar));
   // cv::Mat depth(height, width, CV_32FC1, buffer->data_depth, buffer->width * sizeof (float));

    // Run thread
    std::thread grab_thread(grab_run);
    char key = ' ';
    std::cout << "Press 'q' to exit" << std::endl;


    int count = 1;
    string s1 ="/media/ubuntu/reserved2/images/left";
    string s2 ="/media/ubuntu/reserved2/images/right";
    // string s3 ="/media/ubuntu/reserved1/images/depth";
    string spath;

    // loop until 'q' is pressed
    while (key != 'q')
    {
       sleep(10);
       if (newFrame)                           //等待"生产者"产生数据完毕
       {
          newFrame = false;  
                                                
          time_t start_time,end_time;         //define writing picture time
          ofstream outfile;
          outfile.open("writePicturefile.txt");
          start_time = clock();
				// Retrieve data from buffer
				// buffer->mutex_input_depth.try_lock();
				// memcpy((float*) depth.data, buffer->data_depth, buffer->width * buffer->height * sizeof (float));
				// buffer->mutex_input_depth.unlock();
/********************************************************************************************/
// 改动点：注释一行，添加一行
// 时间：2018/10/18
//	作者：张旭东
// 说明：上强制锁，在没有unlock时无线等待，保证左帧图像的数据被复制完之后再进行其他操作
/*******************************************************************************************/
            // buffer->mutex_input_imagel.try_lock();
			  buffer->mutex_input_imagel.lock();
/*******************************************************************************************/
           memcpy(left.data, buffer->data_iml, buffer->width * buffer->height * buffer->im_channels * sizeof (uchar));
/*******************************************************************************************/
// 改动点：增加以下两行
// 时间：2018/10/18
//	作者：张旭东
// 说明：1.spath存储文件路径
//       2.imwrite 调用OpenCV API将左帧数据存成文件，文件格式为BMP
/*******************************************************************************************/			  
			  spath = s1 + to_string(count) + ".bmp";
           imwrite(spath, left);
/*******************************************************************************************/
           buffer->mutex_input_imagel.unlock();

/********************************************************************************************/
// 改动点：注释一行，添加一行
// 时间：2018/10/18
//	作者：张旭东
// 说明：上强制锁，在没有unlock时无线等待，保证右帧图像的数据被复制完之后再进行其他操作
/*******************************************************************************************/
           // buffer->mutex_input_imager.try_lock();
			  buffer->mutex_input_imager.lock();
/*******************************************************************************************/			  
           memcpy(right.data, buffer->data_imr, buffer->width * buffer->height * buffer->im_channels * sizeof (uchar));
/*******************************************************************************************/
// 改动点：增加以下两行
// 时间：2018/10/18
//	作者：张旭东
// 说明：1.spath存储文件路径
//       2.imwrite 调用OpenCV API将右帧数据存成文件，文件格式为BMP
/*******************************************************************************************/
			  spath = s2 + to_string(count) + ".bmp";
           imwrite(spath, right);
/*******************************************************************************************/			  
           buffer->mutex_input_imager.unlock();

/*******************************************************************************************/
// 改动点：删除一下四行
// 时间：2018/10/18
//	作者：张旭东
/*******************************************************************************************/
				// spath = s1 + to_string(count) + ".bmp";
				// imwrite(spath, left);
				// spath = s2 + to_string(count) + ".bmp";
				// imwrite(spath, right);
/*******************************************************************************************/			  
			  
				// spath = s3 + to_string(count) + ".bmp";
				//imwrite(spath, depth);


				// Do stuff
				//cv::namedWindow("Left", CV_WINDOW_NORMAL);
				//cv::imshow("Left", left);
				//key = cv::waitKey(1);

				//cv::namedWindow("Left", CV_WINDOW_NORMAL);
				//cv::imshow("Right", right);
				//key = cv::waitKey(1);

				//cv::namedWindow("Left", CV_WINDOW_NORMAL);
				//cv::imshow("Depth", depth);
				//key = cv::waitKey(10);
           end_time = clock();
           cout << "The write time is:" << (end_time - start_time)/1000 << "us!" << endl;
           outfile << "writePictureTime:" << (end_time - start_time)/1000 << "us!" << endl;
           outfile.close();
           count += 1;
			  
			  stop_signal = false;  //"消费者"消费数据完毕，通知"生产者"再次生产数据

        }
        else
       {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
       }

    }
    // Stop the grabbing thread
    // stop_signal = true;
    grab_thread.join();

   // delete[] buffer->data_depth;
    delete[] buffer->data_iml;
    delete[] buffer->data_imr;
    delete buffer;
    delete zed;
    return 0;
}



