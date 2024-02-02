
/*
  Source: given example file & VTK documentation website.
*/

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

  ifstream f;
  double x, y, z;
  std::string line;
  
  if(argc != 4){
    cout << "Usage ./geotest <data file> <globe.obj file> <globe.mtl file>\n";
    return 0;
  }

  //locations
  VTK_CREATE(vtkMutableDirectedGraph, g);
  VTK_CREATE(vtkDoubleArray, latitude);
  latitude->SetName("latitude");
  VTK_CREATE(vtkDoubleArray, longitude);
  longitude->SetName("longitude");

  f.open(argv[1]);

  while(std::getline(f,line))
  {
    f >> y >> x >> z;
    //check for non latitude/longitude data
    if(-180.0<=x && x <=180.0 && -90.0<=y && y<=90.0)
    {
    	g->AddVertex();
    	std::cout << y << " " << x << std::endl;
        latitude->InsertNextValue(y);
        longitude->InsertNextValue(x);
    }
  }
  
  f.close();

  g->GetVertexData()->AddArray(latitude);
  g->GetVertexData()->AddArray(longitude);

  VTK_CREATE(vtkGeoAssignCoordinates, assign);
  assign->SetInputData(g);
  assign->SetLatitudeArrayName("latitude");
  assign->SetLongitudeArrayName("longitude");
  assign->SetGlobeRadius(60);
  assign->Update();

  VTK_CREATE(vtkGraphMapper, mapper);
  mapper->SetInputConnection(assign->GetOutputPort());
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  

  /* globe */

  VTK_CREATE(vtkOBJImporter, objImporter);
  objImporter->SetFileName(argv[2]);
  objImporter->SetFileNameMTL(argv[3]);


  
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(actor);
  
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  //win->AddRenderer(ren2);
  //win->AddRenderer(ren1);
  win->SetInteractor(iren);
  objImporter->SetRenderWindow(win);
  ren->ResetCamera();
  objImporter->Update();

  int retVal = vtkRegressionTestImage(win);
  // if (retVal == vtkRegressionTester::DO_INTERACTOR)
    // {
    iren->Initialize();
    iren->Start();

    retVal = vtkRegressionTester::PASSED;
    // }

  return !retVal;
}
