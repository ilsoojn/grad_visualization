//C & C++
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

//openCV
#include <opencv2/opencv.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/video/background_segm.hpp> //BackgroundSubtractor
#include <opencv2/imgproc.hpp>

// arguments : videoName
cv::Mat inframe, frame, bgsub, prevframe; //inputFrame, frame, background subtraction result, previous frame

cv::Ptr<cv::BackgroundSubtractorMOG2> mog;

void inputvideo(char* invf, char* outvf){

	//input video
	cv::VideoCapture inVideo(invf);
	if(!inVideo.isOpened()){
		std::cerr << "Can't Open the video file " << invf << std::endl;
		exit(EXIT_FAILURE);
	}

	cv::Size outputSize = cv::Size((int) inVideo.get(CV_CAP_PROP_FRAME_WIDTH),(int) inVideo.get(CV_CAP_PROP_FRAME_HEIGHT));

	cv::VideoWriter outVideo(outvf, (int)inVideo.get(CV_CAP_PROP_FOURCC), inVideo.get(CV_CAP_PROP_FPS), outputSize, false); // colour = false
	if(!outVideo.isOpened()){
		std::cerr << "Can't Open the video file " << outvf << std::endl;
		exit(EXIT_FAILURE);
	}

    int keyboard;

    //read input data. EPSC or 'q' for quitting
    while( (char)keyboard != 'q' || keyboard == 27){

        //read the current frame
        if(!inVideo.read(inframe)) {
            std::cerr << "Unable to read next frame." << invf << std::endl;
            exit(EXIT_FAILURE);
        }

        //get previous frame
        if(frame.empty() == false){
        	frame.copyTo(prevframe);
        }
        inframe.copyTo(frame);

        /****************************************************************/

				int n = 0;
				int iter = 10;
				cv::Mat gblur; // Gaussian Blur
        cv::GaussianBlur(frame, gblur, cv::Size(n*2+1, n*2+1), 1); // Gaussian Blur

        //update the background model & background/foreground segmentation
        cv::Mat fg, bg;
        mog->apply(frame, bgsub); // Background Subtraction
        frame.copyTo(fg, bgsub);
        mog->getBackgroundImage(bg);

        // gt previous background frame
        cv::Mat prevbg;
        if(bg.empty() == false){
        	bg.copyTo(prevbg);
        }

        /****************************************************************/

        //black white frame
        cv::Mat bwframe, bwfg, bwbg, bwprev, bwprevbg;

        cv::cvtColor(frame, bwframe, cv::COLOR_BGR2GRAY);
        cv::cvtColor(fg, bwfg, cv::COLOR_BGR2GRAY);
        cv::cvtColor(bg, bwbg, cv::COLOR_BGR2GRAY);
        cv::cvtColor(prevbg, bwprevbg, cv::COLOR_BGR2GRAY);
        if(prevframe.empty() == false){
        	cv::cvtColor(prevframe, bwprev, cv::COLOR_BGR2GRAY);
        }

        /****************************************************************/
        /*--------------------------------------------------------------*/
        /*-------------------- B A C K G R O U N D ---------------------*/
        /*--------------------------------------------------------------*/
        /****************************************************************/

        //Optical Flow
        int maxCorners = 1;
        double qualityLevel = 0.01, minDistance = 1;

        std::vector<cv::Point2f> prevPts, nextPts; // previous & next point
        std::vector<uchar> status;
        std::vector<float> err;

        cv::goodFeaturesToTrack(bwprevbg, prevPts, maxCorners, qualityLevel, minDistance, cv::Mat(), 3, true, 0.04);
        cv::cornerSubPix(bwprevbg, prevPts, cv::Size(10,10), cv::Size(-1,-1), cv::TermCriteria());
        cv::calcOpticalFlowPyrLK(bwprevbg, bwframe, prevPts, nextPts, status, err);

        // Drawing Red dot on the frame
        /*for(size_t op = 0; op < nextPts.size(); op++){
        	cv::circle( frame, nextPts[op], 10, cv::Scalar(0, 0, 200), -1 );
        }*/

        // Check if the previous point & next point locations are different
        std::vector<float> subvec;
        subvec.push_back(abs(nextPts.at(0).x - prevPts.at(0).x));
        subvec.push_back(abs(nextPts.at(0).y - prevPts.at(0).y));

        // check if the optical flow point moved; if so, reset background subtractor
        if(subvec.at(0) > 1 && subvec.at(1) > 1){//} && int(inVideo.get(cv::CAP_PROP_POS_FRAMES)) % 3 == 0){
        	mog = cv::createBackgroundSubtractorMOG2(200, 16, false);
        	mog->setDetectShadows(false);
			    mog->setVarThreshold(100.0);
			    mog->setVarThresholdGen(80.0);
        }

        /****************************************************************/

				//Histogram Compare - original frame & foreground frame
				//double comphist_correl = 0, comphist_chisqr = 0, comphist_intersection = 0;
				double comphist_bhatt = 0;

        if(prevframe.empty() == false){
	        cv::MatND prevhist, orighist, fghist;
            int nimages = 2;
	        int channels[] = {0,1};
	        int histSize[] = {32,32};
	        float range[] = {0,256};
	        const float* ranges[] = {range, range};
	        cv::calcHist(&bwfg, nimages, channels, cv::Mat(), fghist, 2, histSize, ranges);
	        cv::calcHist(&bwframe, nimages, channels, cv::Mat(), orighist, 2, histSize, ranges);
	        /*comphist_correl = cv::compareHist(orighist, fghist, CV_COMP_CORREL);
	        comphist_chisqr = cv::compareHist(orighist, fghist, CV_COMP_CHISQR);
	        comphist_intersection = cv::compareHist(orighist, fghist, CV_COMP_INTERSECT);*/
	        comphist_bhatt = cv::compareHist(orighist, fghist, CV_COMP_BHATTACHARYYA);
	    	}

        /****************************************************************/
        /*--------------------------------------------------------------*/
        /*-------------------- F O R E G R O U N D ---------------------*/
        /*--------------------------------------------------------------*/
        /****************************************************************/

        //Morphology Opening
        cv::Mat opendst; // Opening Morphology
        cv::Mat kelement = getStructuringElement(cv::MORPH_RECT, cv::Size(n*2+1,n*2+1), cv::Point(n,n));
        cv::morphologyEx(bgsub, opendst, cv::MORPH_OPEN, kelement, cv::Point(-1,-1), iter);
        //cv::Mat erodeke = getStructuringElement(cv::MORPH_RECT, cv::Size(3,3), cv::Point(n,n));
        //cv::erode(bgsub, opendst, erodeke);

		    /****************************************************************/
		    /*--------------------------------------------------------------*/
        /*------------------ Background & Foreground -------------------*/
        /*--------------------------------------------------------------*/
        /****************************************************************/
        cv::Mat canny, result;
		    int x = 1, y = 1, k = 1;
		    double scale = 10, delta = 0;

		    if(comphist_bhatt < 0.90){

		    	//cv::Laplacian(frame, temp, -1, k);
		    	cv::Canny(frame,canny,50,200,3,true);
	            //cv::add(opendst, canny, result);
		    	outVideo.write(canny);
		    }else{

		    	//cv::Laplacian(bg, temp, -1, k);
		    	cv::Canny(bg,canny,50,200,3,true);
          cv::add(opendst, canny, result);
		    	outVideo.write(result);
		    	//outVideo.write(lastscene);
				}

        /****************************************************************/
        /*--------------------------------------------------------------*/
        /*-------------------- B A C K G R O U N D ---------------------*/
        /*--------------------------------------------------------------*/
        /****************************************************************/

					// Contour on original frame
        // source: openCV doc. page : /tutorials/imgproc/shapedescriptors/bounding_rects_circles/bounding_rects_circles.html
				cv::Mat temp, thframe;
				std::vector<std::vector<cv::Point> > contour;
        std::vector<cv::Vec4i> hierarchy;

        //cv::cvtColor(frame, temp, cv::COLOR_BGR2GRAY);

				cv::threshold(bwfg, thframe, 50, 70, CV_THRESH_BINARY);
        cv::findContours(thframe, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE); // Contours
        if(!contour.empty()){
        	std::vector<cv::Rect> boundR(contour.size());
          std::vector<std::vector<cv::Point> > poly(contour.size());

        	for(int i = 0; i < contour.size(); i++){
            cv::approxPolyDP(cv::Mat(contour[i]), poly[i], 1, true);
        		boundR[i] = cv::boundingRect(cv::Mat(poly[i]));
        	}

            // Drawing Rectangle contours
          cv::Mat draw = cv::Mat::zeros(thframe.size(), CV_8UC3);
        	for(int i = 0; i < contour.size(); i++){
            if(boundR[i].tl() != boundR[i].br()){
                rectangle(draw, boundR[i].tl(), boundR[i].br(), cv::Scalar(0,0,255), 2,8,0);
            }
          }
          cv::namedWindow("Rectangle Contours");
          cv::imshow("Rectangle Conoutrs", draw);

        }//if(contour)

        /****************************************************************/
        /*--------------------------------------------------------------*/
        /*----------------------- W I N D O W S ------------------------*/
        /*--------------------------------------------------------------*/
        /****************************************************************/

        //get the frame number and write it on the current frame
        std::stringstream ss;
        rectangle(frame, cv::Point(10, 2), cv::Point(100,20), cv::Scalar(255,255,255), -1);
        ss << inVideo.get(cv::CAP_PROP_POS_FRAMES);
        std::string frameNumberString = ss.str();
        putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
                cv::FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
        /*source: openCV doc example: read & write image and video*/

        /****************************************************************/

        //show
        if(!canny.empty()){ cv::imshow("Background Canny Edge Detection", canny);}
        if(!opendst.empty()){ cv::imshow("Foreground Background Subtraction", opendst);}
        if(!result.empty()){ cv::imshow("Visual Image Process", result);}
        if(!frame.empty()){ cv::imshow("Original Video", frame);}

        //get the input from the keyboard
        keyboard = cv::waitKey( 30 );
    }

    inVideo.release();
}

int main(int argc, char* argv[]){

	if(argc != 3){
		std::cerr << "Input Error. Usage: ./bgsubtraction <input video file> <output video file name>"<<std::endl;
		return EXIT_FAILURE;
	}

	cv::namedWindow("Foreground Background Subtraction");
	cv::namedWindow("Background Canny Edge Detection");
	cv::namedWindow("Visual Image Process");
	cv::namedWindow("Original Video");

    mog = cv::createBackgroundSubtractorMOG2(200, 16, false);
    mog->setDetectShadows(false);
    mog->setVarThreshold(100.0);
    mog->setVarThresholdGen(80.0);

	// read & process video
	inputvideo(argv[1], argv[2]);

	// write & wdevelop video
	/*cv::VideoWriter vw;
	vw.open(NewVideoName, )
*/
	return 0;
}
