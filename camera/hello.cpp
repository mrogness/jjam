#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <libcamera/libcamera.h>

using namespace libcamera;
using namespace std::chrono_literals;

// Required to store and share all available cameras through libcamera
static std::shared_ptr<Camera> camera;

int main(){
    // Create CameraManager
    std::unique_ptr<CameraManager> camManager = std::make_unique<CameraManager>();
    camManager->start();

    // Get, print all available cameras
    auto cameras = camManager->cameras();
    std::cout << "Available cameras:" << std::endl;
    for(const auto &camera : cameras){
        std::cout << camera->id() << std::endl;
    }
    std::cout << std::endl;

    // Select first camera
    //
    // Stop with error code if none are available
    if(cameras.empty()){
        std::cout << "No cameras available..." << std::endl;
        camManager->stop();
        return 1;
    }
    std::string camID = cameras[0]->id();
    camera = camManager->get(camID);
    camera->acquire(); // Acquire lock

    // Configure
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration({ StreamRole::StillCapture });
    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default StillCapture config: " << streamConfig.toString() << std::endl;
    /*
    Height/width adjust:
    streamConfig.size.width = 640;
    streamConfig.size.height = 480;
    */

    config->validate();
    std::cout << "Validated config: " << streamConfig.toString() << std::endl;
    camera->configure(config.get());

    // Cleanup
    camera->stop();
    camera->release();
    camera.reset();
    camManager->stop();
    return 0;
}