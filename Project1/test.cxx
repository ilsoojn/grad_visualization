 /*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoAssignCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGeoAssignCoordinates.h"
#include "vtkGraphMapper.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vtkOBJReader.h>
#include <vtkOBJImporter.h>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int main(int argc, char *argv[])
{

  double x, y, z;
  std::string line;
  
  if(argc != 3){
    cout << "Usage ./test <globe.obj file> <globe.mtl file>\n";
    return 0;
  }

  //locations
  VTK_CREATE(vtkMutableDirectedGraph, g);
  VTK_CREATE(vtkDoubleArray, latitude);
  latitude->SetName("latitude");
  VTK_CREATE(vtkDoubleArray, longitude);
  longitude->SetName("longitude");

  double latWSU = 39.7798;
  double longWSU = -84.0648;
  latitude->InsertNextValue(latWSU);
  longitude->InsertNextValue(longWSU);

  g->GetVertexData()->AddArray(latitude);
  g->GetVertexData()->AddArray(longitude);

  VTK_CREATE(vtkGeoAssignCoordinates, assign);
  assign->SetInputData(g);
  assign->SetLatitudeArrayName("latitude");
  assign->SetLongitudeArrayName("longitude");
  assign->SetGlobeRadius(40);
  assign->Update();

  VTK_CREATE(vtkGraphMapper, mapper);
  mapper->SetInputConnection(assign->GetOutputPort());
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  
  /*VTK_CREATE(vtkRenderer, ren1);
  ren1->AddActor(actor1);  */

  /* globe */

  /*VTK_CREATE(vtkOBJImporter, objImporter);
  objImporter->SetFileName(argv[1]);
  objImporter->SetFileNameMTL(argv[2]);*/

  /*VTK_CREATE(vtkGraphMapper, mapper2);
  mapper2->SetInputConnection(objReader->GetOutputPort());
  VTK_CREATE(vtkActor, actor2);
  actor2->SetMapper(mapper2);
  VTK_CREATE(vtkRenderer, ren2);
  ren2->AddActor(actor2);  
*/
  
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(actor);
  
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  //objImporter->SetRenderWindow(win);
  ren->ResetCamera();
  //objImporter->Update();

  int retVal = vtkRegressionTestImage(win);
  // if (retVal == vtkRegressionTester::DO_INTERACTOR)
    // {
    iren->Initialize();
    iren->Start();

    retVal = vtkRegressionTester::PASSED;
    // }

  return !retVal;
}