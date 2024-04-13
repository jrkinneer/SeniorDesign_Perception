// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

/* Include the librealsense C header files */
#include <librealsense2/rs.h>
#include <librealsense2/h/rs_pipeline.h>
#include <librealsense2/h/rs_frame.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "example.h"
#include "qrCode.h"
#include <sys/time.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     These parameters are reconfigurable                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define STREAM          RS2_STREAM_COLOR  // rs2_stream is a types of data provided by RealSense device           //
#define FORMAT          RS2_FORMAT_RGB8   // rs2_format identifies how binary data is encoded within a frame      //
#define WIDTH           1920               // Defines the number of columns for each frame                         //
#define HEIGHT          1080               // Defines the number of lines for each frame                           //
#define FPS             30                // Defines the rate of frames per second                                //
#define STREAM_INDEX    0                 // Defines the stream index, used for multiple streams of the same type //
#define WINDOW_WIDTH    540
#define WINDOW_HEIGHT   540
#define STEP_SIZE_LR    60                //make sure that step size is a multiple of 
#define STEP_SIZE_UD    60
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
    rs2_error* e = 0;

    // Create a context object. This object owns the handles to all connected realsense devices.
    // The returned object should be released with rs2_delete_context(...)
    rs2_context* ctx = rs2_create_context(RS2_API_VERSION, &e);
    check_error(e);

    /* Get a list of all the connected devices. */
    // The returned object should be released with rs2_delete_device_list(...)
    rs2_device_list* device_list = rs2_query_devices(ctx, &e);
    check_error(e);

    int dev_count = rs2_get_device_count(device_list, &e);
    check_error(e);
    printf("There are %d connected RealSense devices.\n", dev_count);
    if (0 == dev_count)
        return EXIT_FAILURE;

    // Get the first connected device
    // The returned object should be released with rs2_delete_device(...)
    rs2_device* dev = rs2_create_device(device_list, 0, &e);
    check_error(e);

    print_device_info(dev);

    // Create a pipeline to configure, start and stop camera streaming
    // The returned object should be released with rs2_delete_pipeline(...)
    rs2_pipeline* pipeline =  rs2_create_pipeline(ctx, &e);
    check_error(e);

    // Create a config instance, used to specify hardware configuration
    // The retunred object should be released with rs2_delete_config(...)
    rs2_config* config = rs2_create_config(&e);
    check_error(e);

    // Request a specific configuration
    rs2_config_enable_stream(config, STREAM, STREAM_INDEX, WIDTH, HEIGHT, FORMAT, FPS, &e);
    check_error(e);

    // Start the pipeline streaming
    // The retunred object should be released with rs2_delete_pipeline_profile(...)
    rs2_pipeline_profile* pipeline_profile = rs2_pipeline_start_with_config(pipeline, config, &e);
    if (e)
    {
        printf("The connected device doesn't support color streaming!\n");
        exit(EXIT_FAILURE);
    }

    //variable declaration
    int skip_index = 0;
    std::vector<cv::Point> qrCode_pixel_cords;

    while (1)
    {
        struct timeval stop, start;
        gettimeofday(&start, NULL);

        // This call waits until a new composite_frame is available
        // composite_frame holds a set of frames. It is used to prevent frame drops
        // The returned object should be released with rs2_release_frame(...)
        rs2_frame* frames = rs2_pipeline_wait_for_frames(pipeline, RS2_DEFAULT_TIMEOUT, &e);
        check_error(e);

        // Returns the number of frames embedded within the composite frame
        int num_of_frames = rs2_embedded_frames_count(frames, &e);
        check_error(e);

        //this skips ten frames to make sure enough light gets into the aperature
        if (skip_index < 10){
            skip_index++;
            continue;
        }

        int i;
        for (i = 0; i < num_of_frames; ++i)
        {
            
            // The retunred object should be released with rs2_release_frame(...)
            rs2_frame* frame = rs2_extract_frame(frames, i, &e);
            check_error(e);

            const uint8_t* rgb_frame_data = (const uint8_t*)(rs2_get_frame_data(frame, &e));
            check_error(e);

            //put image into cv::Mat format to make it compatable with the rest of the code
            cv::Mat rgb_image(cv::Size(WIDTH, HEIGHT), CV_8UC3, (void*)rgb_frame_data);

            // Convert BGR to RGB color channel order
            cv::cvtColor(rgb_image, rgb_image, cv::COLOR_BGR2RGB);
            std::cout<<"new image"<<std::endl;
            
            // cv::imshow("raw image", rgb_image);
            // cv::waitKey(3000);
            // cv::destroyAllWindows();

            //window search
            cv::Mat window(WINDOW_WIDTH, WINDOW_HEIGHT, CV_8UC3);

            bool break_loop = false;
            
            for (int i = 0; i <= HEIGHT - WINDOW_HEIGHT; i += STEP_SIZE_UD){
                for (int j = 0; j <= WIDTH - WINDOW_WIDTH; j += STEP_SIZE_LR){
                    //get sliding window
                    cv::Rect roi(j, i, WINDOW_WIDTH, WINDOW_HEIGHT);
                    // Extract the window
                    window = rgb_image(roi);
                    std::cout<<"new window"<<std::endl;
                    if (qrCodeDetect(i, j, window, qrCode_pixel_cords)){ //detect qr code
                        if (errorCheckBoundingBox(qrCode_pixel_cords, j, i, WINDOW_WIDTH, WINDOW_HEIGHT)){ //error check qr detection
                            std::cout<<"found"<<std::endl;
                            break_loop = !break_loop;
                            cubeData cube = getBoundingCube(window, qrCode_pixel_cords);
                            
                            //visual debugging tools
                            // drawBoundingBox(rgb_image, cube.pixel);
                            // drawPoints(rgb_image, qrCode_pixel_cords);
                            std::cout<<cube.xyz<<std::endl;
                            // cv::imshow("found in window", window);
                            // cv::waitKey(5000);
                            // cv::destroyAllWindows();
                        }
                    }

                    if (break_loop){
                        break;
                    }
                }
                if (break_loop){
                    break;
                }
            }   
            rs2_release_frame(frame);
            gettimeofday(&stop, NULL);
            printf("took %lu us for window processing\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
            bool stop = true;
        }

        rs2_release_frame(frames);
    }

    // Stop the pipeline streaming
    rs2_pipeline_stop(pipeline, &e);
    check_error(e);

    // Release resources
    rs2_delete_pipeline_profile(pipeline_profile);
    rs2_delete_config(config);
    rs2_delete_pipeline(pipeline);
    rs2_delete_device(dev);
    rs2_delete_device_list(device_list);
    rs2_delete_context(ctx);

    return EXIT_SUCCESS;
}