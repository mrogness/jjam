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

    // Print all available cameras
    for(const auto &camera : camManager->cameras()){
        std::cout << camera->id() << std::endl;
    }

    return 0;
}