#include "CameraViewModel.hpp"

namespace PathFinder
{

    void CameraViewModel::Import()
    {
        mCamera = &Dependencies->ScenePtr->MainCamera();

        FOVH = mCamera->FOVH();
        LenseAperture = mCamera->Aperture();
        FilmSpeed = mCamera->FilmSpeed();
        ShutterTime = 1.0 / mCamera->ShutterTime();
        View = mCamera->View();
        Projection = mCamera->Projection();
    }

    void CameraViewModel::Export()
    {
        mCamera->SetFieldOfView(FOVH);
        mCamera->SetAperture(LenseAperture);
        mCamera->SetFilmSpeed(FilmSpeed);
        mCamera->SetShutterTime(1.0 / ShutterTime);
    }

}
