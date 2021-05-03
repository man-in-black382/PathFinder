#include "CameraViewModel.hpp"

namespace PathFinder
{

    void CameraViewModel::Import()
    {
        mCamera = &Dependencies->ScenePtr->GetMainCamera();

        FOVH = mCamera->GetFOVH();
        LenseAperture = mCamera->GetAperture();
        FilmSpeed = mCamera->GetFilmSpeed();
        ShutterTime = 1.0 / mCamera->GetShutterTime();
        View = mCamera->GetView();
        Projection = mCamera->GetProjection();
    }

    void CameraViewModel::Export()
    {
        mCamera->SetFieldOfView(FOVH);
        mCamera->SetAperture(LenseAperture);
        mCamera->SetFilmSpeed(FilmSpeed);
        mCamera->SetShutterTime(1.0 / ShutterTime);
    }

}
