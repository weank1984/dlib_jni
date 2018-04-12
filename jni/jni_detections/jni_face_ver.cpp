#include <android/bitmap.h>
#include <jni_common/jni_fileutils.h>
#include <jni_common/jni_utils.h>
#include <verification.h>
#include <jni.h>

#define CLASSNAME_FACE_VER "com/example/weank/smartalbum/dlib/FaceVerify"

using namespace cv;

namespace {

#define JAVA_NULL 0
#define DLIB_VEC_DIMENSION  128

using VerifyPtr = DlibFaceVerify*;

class JNI_FaceVer {
 public:
 	JNI_FaceVer(JNIEnv* env) {
 		jclass clazz = env->FindClass(CLASSNAME_FACE_VER);
 		mNativeContext = env->GetFieldID(clazz, "mNativeContext", "J");
 		env->DeleteLocalRef(clazz);
 		DLOG(INFO) << "JNI_FaceVer in";
 	}

 	VerifyPtr getVerifyPtrFromJava(JNIEnv* env, jobject thiz) {
 		VerifyPtr const p = (VerifyPtr)env->GetLongField(thiz, mNativeContext);
 		return p;
 	}

 	void setVerifyPtrToJava(JNIEnv* env, jobject thiz, jlong ptr) {
 		env->SetLongField(thiz, mNativeContext, ptr);
 	}

 	jfieldID mNativeContext;
};


std::mutex gLock;

std::shared_ptr<JNI_FaceVer> getJNI_FaceVer(JNIEnv* env) {
  static std::once_flag sOnceInitflag;
  static std::shared_ptr<JNI_FaceVer> sJNI_FaceVer;
  std::call_once(sOnceInitflag, [env]() {
    sJNI_FaceVer = std::make_shared<JNI_FaceVer>(env);
  });
  return sJNI_FaceVer;
}

VerifyPtr const getVerifyPtr(JNIEnv* env, jobject thiz) {
  std::lock_guard<std::mutex> lock(gLock);
  return getJNI_FaceVer(env)->getVerifyPtrFromJava(env, thiz);
}


void setVerifyPtr(JNIEnv* env, jobject thiz, VerifyPtr newPtr) {
  std::lock_guard<std::mutex> lock(gLock);
  VerifyPtr oldPtr = getJNI_FaceVer(env)->getVerifyPtrFromJava(env, thiz);
  if (oldPtr != JAVA_NULL) {
    DLOG(INFO) << "setMapManager delete old ptr : " << oldPtr;
    delete oldPtr;
  }

  if (newPtr != JAVA_NULL) {
    DLOG(INFO) << "setMapManager set new ptr : " << newPtr;
  }

  getJNI_FaceVer(env)->setVerifyPtrToJava(env, thiz, (jlong)newPtr);
}

}

#ifdef __cplusplus
extern "C" {
#endif

#define DLIB_FACE_JNI_FV_METHOD(METHOD_NAME) \
  Java_com_example_weank_smartalbum_dlib_FaceVerify_##METHOD_NAME


void JNIEXPORT
    DLIB_FACE_JNI_FV_METHOD(jniNativeClassFVInit)(JNIEnv* env, jclass _this) {
    LOG(INFO) << "jniNativeClassFVInit in";
    }

jobjectArray getVerifyResult(JNIEnv* env, VerifyPtr faceVerify)
{
    LOG(INFO) << "getVerifyResult in";

    jobjectArray jVer2DArray;
    std::vector<matrix<float,0,1>> res = faceVerify->getVerResult();
    jclass floatArrayClass = env->FindClass("[F");
    jVer2DArray = env->NewObjectArray(res.size(), floatArrayClass, NULL);
    float *resTemp = new float[DLIB_VEC_DIMENSION];
    for (int i = 0; i < res.size(); i++) {
        for(int j = 0; j < DLIB_VEC_DIMENSION; j++) {
            resTemp[j] = res[i](j);
        }
        jfloatArray jVerArray = env->NewFloatArray(DLIB_VEC_DIMENSION);
        env->SetFloatArrayRegion(jVerArray, 0, DLIB_VEC_DIMENSION, resTemp);
        env->SetObjectArrayElement(jVer2DArray, i, jVerArray);
        env->DeleteLocalRef(jVerArray);
    }
    delete[] resTemp;
    return jVer2DArray;
}

jint JNIEXPORT JNICALL DLIB_FACE_JNI_FV_METHOD(jniFvInit)(JNIEnv* env, jobject thiz,
                                                       jstring jLandmarkPath,
                                                       jstring jFaceVerifyPath) {
  LOG(INFO) << "jniFaceVerify init";
  std::string landmarkPath = jniutils::convertJStrToString(env, jLandmarkPath);
  std::string faceVerifyPath = jniutils::convertJStrToString(env, jFaceVerifyPath);
  VerifyPtr verPtr = new DlibFaceVerify(landmarkPath, faceVerifyPath);
  setVerifyPtr(env, thiz, verPtr);
  return JNI_OK;
}

jint JNIEXPORT JNICALL
    DLIB_FACE_JNI_FV_METHOD(jniFvDeInit)(JNIEnv* env, jobject thiz) {
  LOG(INFO) << "jniDeInit";
  setVerifyPtr(env, thiz, JAVA_NULL);
  return JNI_OK;
}


JNIEXPORT jobjectArray JNICALL
    DLIB_FACE_JNI_FV_METHOD(jniVerify)(JNIEnv* env, jobject thiz,
                                    jstring imgPath) {
  LOG(INFO) << "jniVerify";
  const char* img_path = env->GetStringUTFChars(imgPath, 0);
  VerifyPtr verPtr = getVerifyPtr(env, thiz);
  bool verResult = verPtr->verify(std::string(img_path));
  env->ReleaseStringUTFChars(imgPath, img_path);
  if(!verResult){
      return NULL;
  }
  return getVerifyResult(env, verPtr);
}

#ifdef __cplusplus
}
#endif
