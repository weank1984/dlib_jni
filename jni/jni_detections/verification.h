#include <jni_common/jni_fileutils.h>
#include <dlib/image_loader/load_image.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/image_loader/load_image.h>
#include <glog/logging.h>
#include <jni.h>
#include <memory>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string>
#include <vector>

#include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/dnn.h>
#include <dlib/image_io.h>

using namespace dlib;
using namespace std;

template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual = add_prev1<block<N,BN,1,tag1<SUBNET>>>;

template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual_down = add_prev2<avg_pool<2,2,2,2,skip1<tag2<block<N,BN,2,tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET> 
using block  = BN<con<N,3,3,1,1,relu<BN<con<N,3,3,stride,stride,SUBNET>>>>>;

template <int N, typename SUBNET> using ares      = relu<residual<block,N,affine,SUBNET>>;
template <int N, typename SUBNET> using ares_down = relu<residual_down<block,N,affine,SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256,SUBNET>;
template <typename SUBNET> using alevel1 = ares<256,ares<256,ares_down<256,SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128,ares<128,ares_down<128,SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64,ares<64,ares<64,ares_down<64,SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32,ares<32,ares<32,SUBNET>>>;

using anet_type = loss_metric<fc_no_bias<128,avg_pool_everything<
                            alevel0<
                            alevel1<
                            alevel2<
                            alevel3<
                            alevel4<
                            max_pool<3,3,2,2,relu<affine<con<32,7,7,2,2,
                            input_rgb_image_sized<150>
                            >>>>>>>>>>>>;



// face verification
class DlibFaceVerify {
 private:
  std::string mLandMarkModel;
  std::string mNetModel;
  dlib::frontal_face_detector detector;
  dlib::shape_predictor sp;
  anet_type net;
  std::vector<matrix<float,0,1>> face_descriptors;
  // load model
  inline void load_face_detector()
  {
  	LOG(INFO) << "load_face_detector in";
    detector = dlib::get_frontal_face_detector();
  }

  inline void load_face_landmark(const string& landmark_model){
    mLandMarkModel = landmark_model;
    if (!mLandMarkModel.empty() && jniutils::fileExists(mLandMarkModel)){
        dlib::deserialize(mLandMarkModel) >> sp;
        LOG(INFO) << "load_face_landmark from : " << mLandMarkModel;
    }
  }
  inline void load_face_verify(const string& net_model){
   mNetModel = net_model;
   if (!mNetModel.empty() && jniutils::fileExists(mNetModel)){
        LOG(INFO) << "load_face_verify from : " << mNetModel;
        dlib::deserialize(mNetModel) >> net;
     }
  }

 public:
  DlibFaceVerify()
  {
    load_face_detector();
  }

  DlibFaceVerify(const string& landmark_model, const string& net_model)
  {
    LOG(INFO) << "DlibFaceVerify in";
    load_face_detector();
    load_face_landmark(landmark_model);
    load_face_verify(net_model);
  }

  inline bool verify(const string& image_path)
  {
    LOG(INFO) << "verify image_path" << image_path;
    cv::Mat src_img = cv::imread(image_path, CV_LOAD_IMAGE_COLOR);
    cv::Mat temp, dst;
    temp = src_img;
    // downsample image to reduce running time
    while (temp.rows * temp.cols > 1800 * 1800)
    {
      pyrDown(temp, dst, cv::Size(temp.cols/2, temp.rows/2));
      temp = dst;
    }
    LOG(INFO) << "image size:" << temp.rows << " " << temp.cols;
    return verify(temp);
  }

  inline bool verify(const cv::Mat& image)
  {
    std::vector<matrix<rgb_pixel>> faces;
    std::vector<dlib::rectangle> dets;
    dlib::full_object_detection shape;
    matrix<rgb_pixel> face_chip;
    if (image.empty())
    {
      return false;
    }
    cv_image<dlib::bgr_pixel> img(image);
    LOG(INFO) << "time test: detect start.";
    dets = detector(img);
    LOG(INFO) << "Dlib HOG face det size : " << dets.size();
    if (dets.size() == 0){
        LOG(INFO)  << "No faces found in image!";
        return false;
    }

    for(int i = 0; i < dets.size(); ++i) {
        shape = sp(img, dets[i]);
        extract_image_chip(img, get_face_chip_details(shape,150,0.25), face_chip);
        faces.push_back(move(face_chip));
    }

    face_descriptors = net(faces);
    LOG(INFO) << "Dlib face_descriptors size : " << face_descriptors.size();
    LOG(INFO) << "Dlib face_descriptors[0].nr : " << face_descriptors[0].nr() << "  Dlib face_descriptors[0].nc: " <<face_descriptors[0].nc();
    return true;
  }

  std::vector<matrix<float,0,1>>& getVerResult()
  {
    return face_descriptors;
  }
};
