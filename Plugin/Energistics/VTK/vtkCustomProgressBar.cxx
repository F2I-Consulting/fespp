/*-----------------------------------------------------------------------
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"; you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-----------------------------------------------------------------------*/
#include "vtkCustomProgressBar.h"

#include <chrono>
#include <thread>

//----------------------------------------------------------------------------
vtkCustomProgressBar::vtkCustomProgressBar():
    _isIndeterminate(false),
    _windowName("Processing...")
{
	SetPosition(0.25, 0.5);
	SetProgressBarColor(0.52, 0.76, 0.91);
}

//----------------------------------------------------------------------------
vtkCustomProgressBar* vtkCustomProgressBar::New()
{
    return new vtkCustomProgressBar();
}

void vtkCustomProgressBar::setIndeterminate(bool status)
{
	_isIndeterminate = status;
}

void vtkCustomProgressBar::task1()
{
    const int* screenSize = _window->GetScreenSize();
    int windowWidth = 800; // Largeur de la fenêtre
    int windowHeight = 600; // Hauteur de la fenêtre
    int windowPosX = (screenSize[0] - 150) / 2;
    int windowPosY = (screenSize[1] - 100) / 2;
    _window->SetPosition(windowPosX, windowPosY);
    _window->SetWindowName(_windowName.c_str());
    _window->AddRenderer(_render);
    _renderWindow->SetRenderWindow(_window);
    _widget->SetInteractor(_renderWindow);
    _widget->SetRepresentation(this);
    _renderWindow->Initialize();
    _widget->On();
    double value = 0.0;
    while (_isIndeterminate)
    {
        value += 0.1;

        if (value > 1.0)
        {
            value = 0.0;
        }
        SetProgressRate(value);
        _window->Render();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
