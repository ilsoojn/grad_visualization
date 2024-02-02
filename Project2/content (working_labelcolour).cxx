#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

#include <vtkSmartPointer.h>
#include <vtkGraphLayoutView.h>
#include <vtkDataSetAttributes.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkMutableDirectedGraph.h>
// #include <vtkActor.h>
// #include <vtkPolyDataMapper.h>
// #include <vtkGlyph3D.h>
// #include <vtkGlyphSource2D.h>
// #include <vtkGraphToPolyData.h>
// #include <vtkPolyDataMapper.h>
// #include <vtkSimple2DLayoutStrategy.h>

// #include <vtkGraphLayout.h>
// #include <vtkRenderer.h>

//colour
#include <vtkLookupTable.h>
#include <vtkViewTheme.h>

#include <vtkTable.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>
#include <vtkIntArray.h>
#include <vtkStringArray.h>

vtkSmartPointer<vtkMutableDirectedGraph> g = vtkSmartPointer<vtkMutableDirectedGraph>::New();
vtkSmartPointer<vtkTable> t = vtkSmartPointer<vtkTable>::New(); //[id][name][pid][type]
vtkSmartPointer<vtkLookupTable> lookupT = vtkSmartPointer<vtkLookupTable>::New();

vtkIdType get_pid_str(std::string city){
	int r = 0;
	while(r < t->GetNumberOfRows() && city != t->GetValue(r,1).ToString()){
		r++;
	}

	vtkIdType city_pid = t->GetValue(r,2).ToInt();

	return city_pid;
}//get_pid_str()

vtkIdType get_pid(int id){

	int r = 0;
	
	while(r < t->GetNumberOfRows() && t->GetValue(r,0) != id){
		r++;
	}

	vtkVariant pidv = t->GetValue(r,2);
	vtkIdType pedigree_id = pidv.ToInt();

	return pedigree_id;
}

int create_linkEdge(int id, std::vector<vtkIdType> v){

	if(v.size() == 0){
		return 0;
	}

	vtkIdType pedigree_id = get_pid(id);

	for(int k = 0; k < v.size(); k++){
		g->AddEdge(pedigree_id,v[k]);
	}

	return 1;
}//create_linkEdge()

int create_pplcityEdge(char* pplCity){

	//read input datafile
	std::ifstream pcset(pplCity);
	std::string line;

	while(std::getline(pcset, line)){
		try{
			int id;
			std::string city;
			std::stringstream strstream(line);

			strstream >> id;
			strstream >> city;

			if(id != 0){
				// vtkIdType pid_person = get_pid(id);
				// vtkIdType pid_city = get_pid_str(city);
				// g->AddEdge(pid_person, pid_city);
				g->AddEdge(get_pid(id),get_pid_str(city));
			}

		}catch(std::exception e){
			continue;
		}
	}

	return 1;
}//set_pplCity()

int set_linkTable(char* linkTable){

	//read input datafile
	std::ifstream pointset(linkTable);
	std::string line;

	//EDGE people-people or city-country
	// 
	std::vector <vtkIdType> v;
	int current_id = 0;
	
	while(std::getline(pointset, line)){
		try{

			int from_id, to_id;
			std::stringstream strstream(line);
			
			strstream >> from_id;
			strstream >> to_id;

			if(from_id != 0 && to_id != 0){
				
				// Assume the LinkTable is 
				if(current_id != from_id){

					create_linkEdge(current_id, v);

					current_id = from_id;
					v.clear();
				}
				vtkIdType pedigree_id = get_pid(to_id);
				v.push_back(pedigree_id);
			}
			
		}catch(std::exception d){
			continue;
		}
	}//while

	// link of the last vertex.
	create_linkEdge(current_id,v);
	
	return 0;
}//set_linkTable()

int create_vertex(char* entity){
	
	//read input datafile
	std::ifstream dataset(entity);
	std::string line;

	//create array for each categories
	vtkSmartPointer<vtkIntArray> ID = vtkSmartPointer<vtkIntArray>::New();
	ID->SetName("ID");
	vtkSmartPointer<vtkStringArray> Name = vtkSmartPointer<vtkStringArray>::New();
	Name->SetName("Name");
	vtkSmartPointer<vtkIntArray> PedigreeID = vtkSmartPointer<vtkIntArray>::New();
	PedigreeID->SetName("PedigreeID");
	vtkSmartPointer<vtkStringArray> Type = vtkSmartPointer<vtkStringArray>::New();
	Type->SetName("Type");

	//read the data file and add them into lists/array
	while(std::getline(dataset, line)){

		try{

			int id;
			std::string name,type;
			std::stringstream strstream(line);
			
			strstream >> id;
			strstream >> name;
			strstream >> type;
			
			if(id != 0){
				
				//create a vertex for each data point
				vtkIdType pid = g->AddVertex();

				ID->InsertNextValue(id); 
				Name->InsertNextValue(name);
				PedigreeID->InsertNextValue(pid);
				Type->InsertNextValue(type);
			}//if(id!=0)

		}catch(std::exception d){
			continue;
		}
	}//while

	//add the lists into the table
	t->AddColumn(ID);
	t->AddColumn(Name);
	t->AddColumn(PedigreeID);
	t->AddColumn(Type);

	g->GetVertexData()->AddArray(Name);

	return 0;
}//create_vertex()

int colour(){
	
	lookupT->SetNumberOfTableValues(3);
	lookupT->SetTableValue(0, 1.0,1.0,1.0); // white
	lookupT->SetTableValue(1, 0.0,1.0,0.0); // green
	lookupT->SetTableValue(2, 1.0,0.0,0.0); // red
	lookupT->Build();

	vtkSmartPointer<vtkIntArray> colourArray = vtkSmartPointer<vtkIntArray>::New();
	colourArray->SetNumberOfComponents(1);
	colourArray->SetName("Colour");

	for(int i = 0; i < t->GetNumberOfRows(); i++){
		if("person" == t->GetValue(i,3).ToString()){
			vtkIdType p_pid = t->GetValue(i,2).ToInt();
			colourArray->InsertNextValue(0); // white
		}
		if("city" == t->GetValue(i,3).ToString()){
			vtkIdType c_pid = t->GetValue(i,2).ToInt();
			colourArray->InsertNextValue(1); // green
		}
		if("country" == t->GetValue(i,3).ToString()){
			vtkIdType n_pid = t->GetValue(i,2).ToInt();
			colourArray->InsertNextValue(2); // red
		}
	}

	g->GetVertexData()->AddArray(colourArray);
}

int main(int argc, char *argv[]){
	
	create_vertex(argv[1]);
	//set_linkTable(argv[2]);
	create_pplcityEdge(argv[3]);

	colour();

	vtkSmartPointer<vtkGraphLayoutView> GLview = vtkSmartPointer<vtkGraphLayoutView>::New();
	
	/*
	vtkSmartPointer<vtkGraphLayout> gl = vtkSmartPointer<vtkGraphLayout>::New();
	gl->SetInputData(g);
	vtkSmartPointer<vtkSimple2DLayoutStrategy> strategy = vtkSmartPointer<vtkSimple2DLayoutStrategy>::New();
	gl->SetLayoutStrategy(strategy);

	GLview->SetLayoutStrategyToPassThrough();
	GLview->SetEdgeLayoutStrategyToPassThrough();
	GLview->AddRepresentationFromInputConnection(gl->GetOutputPort());

	vtkSmartPointer<vtkGraphToPolyData> gPoly = vtkSmartPointer<vtkGraphToPolyData>::New();
	gPoly->SetInputConnection(gl->GetOutputPort());
	gPoly->EdgeGlyphOutputOn();
	gPoly->SetEdgeGlyphPosition(0.99);

	vtkSmartPointer<vtkGlyphSource2D> gSource = vtkSmartPointer<vtkGlyphSource2D>::New();
	gSource->SetGlyphTypeToEdgeArrow();
	gSource->SetScale(0.1);
	gSource->Update();

	vtkSmartPointer<vtkGlyph3D> glyph = vtkSmartPointer<vtkGlyph3D>::New();
	glyph->SetInputConnection(0, gPoly->GetOutputPort(1));
	glyph->SetInputConnection(1, gSource->GetOutputPort());

	vtkSmartPointer<vtkPolyDataMapper> pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	pdMapper->SetInputConnection(glyph->GetOutputPort());
	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(pdMapper);
	GLview->GetRenderer()->AddActor(actor);
	*/

	GLview->AddRepresentationFromInput(g);
	GLview->VertexLabelVisibilityOn();
	GLview->SetVertexLabelArrayName("Name");
	GLview->ColorVerticesOn();
	GLview->SetVertexColorArrayName("Colour");
	GLview->Update();

	vtkSmartPointer<vtkViewTheme> theme = vtkSmartPointer<vtkViewTheme>::New();
	theme->SetPointLookupTable(lookupT);
	GLview->ApplyViewTheme(theme);
	
	GLview->ResetCamera();
	GLview->Render();
	GLview->GetInteractor()->Start();

  	return 0;
}//main()

/*
source:


	http://www.vtk.org/Wiki/VTK/Examples/Cxx/Graphs/VisualizeDirectedGraph
	http://www.vtk.org/Wiki/VTK/Examples/Cxx/Graphs/LabelVerticesAndEdges
	http://www.vtk.org/Wiki/VTK/Examples/Cxx/Graphs/ColorVerticesLookupTable

	http://www.vtk.org/doc/nightly/html/classvtkMutableDirectedGraph.html
	http://www.vtk.org/doc/nightly/html/classvtkGraphLayout.html
	http://www.vtk.org/doc/nightly/html/classvtkGraphLayoutView.html

	http://www.vtk.org/doc/nightly/html/classvtkTable.html
	http://www.vtk.org/doc/nightly/html/classvtkVariant.html
*/