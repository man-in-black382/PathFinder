#include "CameraViewModel.hpp"

namespace PathFinder
{

    void CameraViewModel::SetCamera(Camera* camera)
    {
        mCamera = camera;
    }

    void CameraViewModel::Import()
    {
        FOVH = mCamera->FOVH();
        View = mCamera->View();
        Projection = mCamera->Projection();
    }

    void CameraViewModel::Export()
    {
        mCamera->SetFieldOfView(FOVH);
    }

}
