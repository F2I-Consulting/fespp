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
#ifndef __vtkCustomProgressBar_h
#define __vtkCustomProgressBar_h

#include <vtkProgressBarRepresentation.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkProgressBarWidget.h>

class vtkCustomProgressBar : public vtkProgressBarRepresentation {
public:
    vtkCustomProgressBar();
    static vtkCustomProgressBar* New();

    void setIndeterminate(bool status);
    bool getIndeterminate() { return _isIndeterminate; }
    void task1();
    void setWindowName(std::string name) { _windowName = name; }

private:


    bool _isIndeterminate;
    std::string _windowName;
    vtkNew<vtkRenderer> _render;
    vtkNew<vtkRenderWindow> _window;
    vtkNew<vtkRenderWindowInteractor> _renderWindow;
    vtkNew<vtkProgressBarWidget> _widget;
};

#endif
