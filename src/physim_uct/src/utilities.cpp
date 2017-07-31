#include <common_io.h>

namespace utilities{
	
	// Reference: http://answers.opencv.org/question/11788/is-there-a-meshgrid-function-in-opencv/
	static void meshgrid(const cv::Range &xgv, const cv::Range &ygv, cv::Mat1i &X, cv::Mat1i &Y){
		std::vector<int> t_x, t_y;
		for (int i = xgv.start; i <= xgv.end; i++) t_x.push_back(i);
		for (int i = ygv.start; i <= ygv.end; i++) t_y.push_back(i);
		cv::repeat(cv::Mat(t_x).reshape(1,1), cv::Mat(t_y).total(), 1, X);
		cv::repeat(cv::Mat(t_y).reshape(1,1).t(), 1, cv::Mat(t_x).total(), Y);
	} // function: meshgrid

	// Reference: https://stackoverflow.com/questions/10167534/how-to-find-out-what-type-of-a-mat-object-is-with-mattype-in-opencv
	std::string type2str(int type){
	std::string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);
	switch (depth){
		case CV_8U:  r = "8U"; break;
		case CV_8S:  r = "8S"; break;
		case CV_16U: r = "16U"; break;
		case CV_16S: r = "16S"; break;
		case CV_32S: r = "32S"; break;
		case CV_32F: r = "32F"; break;
		case CV_64F: r = "64F"; break;
		default:     r = "User"; break;
	}
    r += "C";
    r += (chans+'0');
	return r;
	} //function: type2str

	void readDepthImage(cv::Mat &depthImg, std::string path){
		cv::Mat depthImgRaw = cv::imread(path, CV_16UC1);
		depthImg = cv::Mat::zeros(depthImgRaw.rows, depthImgRaw.cols, CV_32FC1);
		for(int u=0; u<depthImgRaw.rows; u++)
			for(int v=0; v<depthImgRaw.cols; v++){
				unsigned short depthShort = depthImgRaw.at<unsigned short>(u,v);
				depthShort = (depthShort << 13 | depthShort >> 3);
				float depth = (float)depthShort/10000;
				depthImg.at<float>(u, v) = depth;
			}
	}//function: readDepthImage

	void writeDepthImage(cv::Mat &depthImg, std::string path){
		cv::Mat depthImgRaw = cv::Mat::zeros(depthImg.rows, depthImg.cols, CV_16UC1);
		for(int u=0; u<depthImg.rows; u++)
			for(int v=0; v<depthImg.cols; v++){
				float depth = depthImg.at<float>(u,v)*10000;
				unsigned short depthShort = (unsigned short)depth;
				depthShort = (depthShort >> 13 | depthShort << 3);
				depthImgRaw.at<unsigned short>(u, v) = depthShort;
			}
		cv::imwrite(path, depthImgRaw);
	}//function: writeDepthImage

	// Convert Depth image to point cloud. TODO: Could it be faster?
	// Reference: https://gist.github.com/jacyzon/fa868d0bcb13abe5ade0df084618cf9c
	void convert3d(cv::Mat &objDepth, Eigen::Matrix3f &camIntrinsic, PointCloud::Ptr objCloud){
		int imgWidth = objDepth.cols;
		int imgHeight = objDepth.rows;
		
		objCloud->height = (uint32_t) imgHeight;
		objCloud->width = (uint32_t) imgWidth;
		objCloud->is_dense = false;
		objCloud->points.resize(objCloud->width * objCloud->height);

		// Try meshgrid implementation
		// cv::Mat1i X, Y;
		// meshgrid(cv::Range(1,imgWidth), cv::Range(1, imgHeight), X, Y);
		// printf("Matrix: %s %dx%d \n", utilities::type2str( depth_image.type() ).c_str(), depth_image.cols, depth_image.rows );
		// cv::Mat CamX = ((X-camIntrinsic(0,2)).mul(objDepth))/camIntrinsic(0,0);
		// cv::Mat CamY = ((Y-camIntrinsic(1,2)).mul(objDepth))/camIntrinsic(1,1);

		for(int u=0; u<imgHeight; u++)
			for(int v=0; v<imgWidth; v++){
				float depth = objDepth.at<float>(u,v);
				if(depth > 0.1 && depth < 1.0){
					objCloud->at(v, u).x = (float)((v - camIntrinsic(0,2)) * depth / camIntrinsic(0,0));
					objCloud->at(v, u).y = (float)((u - camIntrinsic(1,2)) * depth / camIntrinsic(1,1));
					objCloud->at(v, u).z = depth;
					std::cout<<"x: "<<objCloud->at(v, u).x<<" y: "<<objCloud->at(v, u).y<<" z: "<<objCloud->at(v, u).z<<std::endl;
				}
		}
	} // function: convert3d

	void convert2d(cv::Mat &objDepth, Eigen::Matrix3f &camIntrinsic, PointCloud::Ptr objCloud){
		for(int i=0;i<objCloud->points.size();i++){
			Eigen::Vector3f point2D = camIntrinsic * 
					Eigen::Vector3f(objCloud->points[i].x, objCloud->points[i].y, objCloud->points[i].z);
			point2D = point2D/point2D[2];
			if(point2D[1] > 0 && point2D[1] < objDepth.rows && point2D[0] > 0 && point2D[0] < objDepth.cols){
				if(objDepth.at<float>(point2D[1],point2D[0]) == 0  || point2D[2] < objDepth.at<float>(point2D[1],point2D[0]))
					objDepth.at<float>(point2D[1],point2D[0]) = (float)point2D[2];
			}
		}
	} //function: convert2d

	// Reference: http://pointclouds.org/documentation/tutorials/pcl_visualizer.php
	boost::shared_ptr<pcl::visualization::PCLVisualizer> simpleVis (PointCloud::ConstPtr cloud){
		boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
		viewer->setBackgroundColor (0, 0, 0);
		viewer->addPointCloud<pcl::PointXYZ> (cloud, "sample cloud");
		viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "sample cloud");
		viewer->initCameraParameters ();
		return (viewer);
	} // function: simpleVis

	void TransformPolyMesh(const pcl::PolygonMesh::Ptr &mesh_in, pcl::PolygonMesh::Ptr &mesh_out, Eigen::Matrix4f transform) {
		PointCloud::Ptr cloud_in (new PointCloud);
		PointCloud::Ptr cloud_out (new PointCloud);
		pcl::fromPCLPointCloud2(mesh_in->cloud, *cloud_in);
		pcl::transformPointCloud(*cloud_in, *cloud_out, transform);
		*mesh_out = *mesh_in;
		pcl::toPCLPointCloud2(*cloud_out, mesh_out->cloud);
		return;
	} //function: TransformPolyMesh

} // namespace