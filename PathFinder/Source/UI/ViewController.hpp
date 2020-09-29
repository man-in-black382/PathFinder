#pragma once

namespace PathFinder
{
   
    class ViewController
    {
    public:
        virtual void Draw() = 0;
        virtual bool IsInteracting() const { return false; };
    };

}
