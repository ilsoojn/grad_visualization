
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
#include <vtkActor.h>

#include <vtkPolyDataMapper.h>
#include <vtkGlyph3D.h>
#include <vtkGlyphSource2D.h>
#include <vtkGraphToPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSimple2DLayoutStrategy.h>
#include <vtkGraphLayout.h>
#include <vtkRenderer.h>
#include <vtkMutableDirectedGraph.h>

//colour
#include <vtkLookupTable.h>
#include <vtkViewTheme.h>

#include <vtkTable.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>
#include <vtkIntArray.h>
#include <vtkStringArray.h>

vtkSmartPointer<vtkMutableDirectedGraph> g = vtkSmartPointer<vtkMutableDirectedGraph>::New();
vtkSmartPointer<vtkTable> t = vtkSmartPointer<vtkTable>::New(); //[PedigreeID][ID][Name][Type][OUT][City][Suspect]

vtkSmartPointer<vtkLookupTable> lookupT = vtkSmartPointer<vtkLookupTable>::New();
vtkSmartPointer<vtkIdTypeArray> toRemove = vtkSmartPointer<vtkIdTypeArray>::New();

//Hard coded size of the city 
std::string LargeCity = "Koul"; // Leader & maybe Handlers/target
std::string largeCity = "Prounov"; // Leader & maybe Handlers/target
std::string MidsizeCity [3] = {"Kouvnic", "Solvenz", "Kannvic"};
std::string SmallCity[7] = {"Otello", "Transpasko", "Pasko", "Solank", "Sresk", "Tulamuk", "Ryzkland"};
int MidsizeCitySize = 3;
int SmallCitySize = 7;

//get PedigreeID of the vertex with city
vtkIdType get_pid_str(std::string city){
	int r = 0;
	while(r < t->GetNumberOfRows() && city != t->GetValueByName(r,"Name").ToString()){
		r++;
	}

	vtkIdType city_pid = t->GetValueByName(r,"PedigreeID").ToInt();

	return city_pid;
}//get_pid_str()

//get PedigreeID of the vertex with id
vtkIdType get_pid(int id){

	int r = 0;
	
	while(r < t->GetNumberOfRows() && t->GetValueByName(r,"ID") != id){
		r++;
	}

	vtkIdType pedigree_id = t->GetValueByName(r,"PedigreeID").ToInt();
	
	return pedigree_id;
}//get_pid

//get the index on the id on the ID Column from Table
vtkIdType getIndexOf(int id){
	int r =0;
	while(r<t->GetNumberOfRows() && t->GetValueByName(r,"ID") != id){
		r++;
	}
	return r;
}//getIndexOf

//see if the location is mid-size city
int in_MidsizeCity(std::string loc){
	
	for(int i = 0; i < MidsizeCitySize; i++){
		if(MidsizeCity[i] == loc){
			return 1;
		}
	}
	return 0;
}//in_MidsizeCity()

//see if the location is small city.
int in_SmallCity(std::string loc){
	
	for(int i = 0; i < SmallCitySize; i++){
		if(SmallCity[i] == loc){
			return 1;
		}
	}
	return 0;
}//in_SmallCity()

//create edge (people-people or city-country) to the graph
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

//create edge from people to city graph
int create_pplcityEdge(char* pplCity){

	vtkSmartPointer<vtkStringArray> City = vtkSmartPointer<vtkStringArray>::New();
	City->SetName("City");

	int numberOfPeople = 0;

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
				City->InsertNextValue(city);
				g->AddEdge(get_pid(id),get_pid_str(city));
				numberOfPeople++;
			}

		}catch(std::exception e){
			continue;
		}
	}

	int i = 0;
	while(City->GetNumberOfValues() < t->GetNumberOfRows()){
		City->InsertNextValue("0");
	}
	t->AddColumn(City);

	return numberOfPeople++;;
}//set_pplCity()

//read the file and set to create edge to the graph
int set_linkTable(char* linkTable){

	//read input datafile
	std::ifstream pointset(linkTable);
	std::string line;

	//EDGE people-people
	std::vector <vtkIdType> v;
	int current_id = 0;
	vtkAbstractArray * out = t->GetColumnByName("OUT");
	while(std::getline(pointset, line)){
		try{

			int from_id, to_id;
			std::stringstream strstream(line);
			
			strstream >> from_id;
			strstream >> to_id;

			if(from_id != 0 && to_id != 0){
				
				// Assume the LinkTable is 
				if(current_id != from_id){
					
					vtkIdType row = getIndexOf(current_id);
					out->SetVariantValue(row, v.size());
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

//read the file and create vertex for each datal.
int create_vertex(char* entity){
	
	//read input datafile
	std::ifstream dataset(entity);
	std::string line;

	//create array for each categories
	vtkSmartPointer<vtkIntArray> PedigreeID = vtkSmartPointer<vtkIntArray>::New();
	PedigreeID->SetName("PedigreeID");
	vtkSmartPointer<vtkIntArray> ID = vtkSmartPointer<vtkIntArray>::New();
	ID->SetName("ID");
	vtkSmartPointer<vtkStringArray> Name = vtkSmartPointer<vtkStringArray>::New();
	Name->SetName("Name");
	vtkSmartPointer<vtkStringArray> Type = vtkSmartPointer<vtkStringArray>::New();
	Type->SetName("Type");
	vtkSmartPointer<vtkIntArray> OutDegree = vtkSmartPointer<vtkIntArray>::New();
	OutDegree->SetName("OUT");

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

				PedigreeID->InsertNextValue(pid);
				ID->InsertNextValue(id); 
				Name->InsertNextValue(name);
				Type->InsertNextValue(type);
				OutDegree->InsertNextValue(0);
			}//if(id!=0)

		}catch(std::exception d){
			continue;
		}
	}//while

	//add the lists into the table
	t->AddColumn(PedigreeID); //column Index 0
	t->AddColumn(ID); //column Index 1
	t->AddColumn(Name); //column Index 2
	t->AddColumn(Type); //column Index 3
	t->AddColumn(OutDegree);

	g->GetVertexData()->AddArray(Name);

	return 0;
}//create_vertex()

//set lookuptable for colouring
int colourTable(){
	
	lookupT->SetNumberOfTableValues(5);
	lookupT->SetTableValue(0, 1.0,1.0,1.0); // white
	lookupT->SetTableValue(1, 0.0,1.0,0.0); // green
	lookupT->SetTableValue(2, 0.0,1.0,1.0); // light blue
	lookupT->SetTableValue(3, 1.0,0.0,0.0); // red
	lookupT->SetTableValue(4, 1.0,1.0,0.0); //pink
	lookupT->Build();

	return 1;
}//colourTable()

//colour vertecis.
int colour_v(){

	int ppl = 0;
	vtkSmartPointer<vtkIntArray> colourArray = vtkSmartPointer<vtkIntArray>::New();

	colourArray->SetNumberOfComponents(1);
	colourArray->SetName("Colour");

	for(int i = 0; i < t->GetNumberOfRows(); i++){
		std::string type = t->GetValueByName(i,"Type").ToString();
		if("person" == type){
			colourArray->InsertNextValue(0);
			
			ppl++;
		}
		if("city" == type){
			colourArray->InsertNextValue(1);
		}
		if("country" == type){
			colourArray->InsertNextValue(2);
		}
	}

	g->GetVertexData()->AddArray(colourArray);

	return ppl;
}//colour_v()

//
int network(int linksize){
	int leader = 0;
	int handler = 0;
	int employee = 0;
	int middleman = 0;

	for(int i = 0 ; i < linksize; i++){

		vtkIdType pid = t->GetValueByName(i,"PedigreeID").ToInt();
		int degree = g->GetDegree(pid);
		int out = t->GetValueByName(i, "OUT").ToInt();
		int in = degree - out;
		std::string loc = t->GetValueByName(i,"City").ToString();

		if(degree > 101 && (loc == LargeCity || loc == largeCity)){
			// over 100 connection and living in the Large City
			leader++;
		}else if(degree >= 31 && degree <= 41){
			// each handlers has 30-40 connection (+ 1 for city edge)
			handler++;
		}else if(degree >=36 && degree <= 46){
			// employee has around 40 flitters
			employee++;
		}else if(degree >= 3 && degree <=6 && (in_MidsizeCity(loc) == 1 || in_SmallCity(loc) == 1)){
			// each handlers have his own middleman or the handlers share a middleman (1 or 3 in)
			// middleman communicate with one or two others, and one must be the Fearless Leader (1 or 2 out)
			// middleman lives in the small/mid-size city
			middleman++;
		}else{
			toRemove->InsertNextValue(pid);
		}
	}
	std::cout<<"Possible number of (Leader, Handler, Employee, Middleman) = (" 
		<< leader << ", " << handler << ", " << employee<< ", "<< middleman  << ")"<< std::endl;

	g->RemoveVertices(toRemove);
	return 1;
}//network()

int remove_city_country(){

	int tsize = t->GetNumberOfRows()-1;
	for(int i = tsize; i >=0; i--){
		std::string type = t->GetValueByName(i,"Type").ToString();
		int id = t->GetValueByName(i,"ID").ToInt();

		if(type == "city" || type == "country"){
			g->RemoveVertex(t->GetValueByName(i,"PedigreeID").ToInt());
		}
	}
	std::cout<< g->GetNumberOfVertices() <<std::endl;
	return 1;
}//simplify_table()

int main(int argc, char *argv[]){
	
	colourTable();
	
	create_vertex(argv[1]);
	set_linkTable(argv[2]);
	int nppl = create_pplcityEdge(argv[3]);

	network(nppl);
	colour_v();
	remove_city_country();

	vtkSmartPointer<vtkGraphLayoutView> GLview = vtkSmartPointer<vtkGraphLayoutView>::New();
	
	//-----
	vtkSmartPointer<vtkGraphLayout> glayout = vtkSmartPointer<vtkGraphLayout>::New();
	vtkSmartPointer<vtkSimple2DLayoutStrategy> strategy = vtkSmartPointer<vtkSimple2DLayoutStrategy>::New();
	glayout->SetInputData(g);
	glayout->SetLayoutStrategy(strategy);

	GLview->SetLayoutStrategyToSimple2D(); // SetLayoutStrategyToPassThrough();
	GLview->SetEdgeLayoutStrategyToPassThrough();
	GLview->AddRepresentationFromInputConnection(glayout->GetOutputPort());

	vtkSmartPointer<vtkGraphToPolyData> gPoly = vtkSmartPointer<vtkGraphToPolyData>::New();
	gPoly->SetInputConnection(glayout->GetOutputPort());
	gPoly->EdgeGlyphOutputOn();
	gPoly->SetEdgeGlyphPosition(1);

	vtkSmartPointer<vtkGlyphSource2D> gSource = vtkSmartPointer<vtkGlyphSource2D>::New();
	gSource->SetGlyphTypeToEdgeArrow();
	gSource->SetScale(0.008);
	gSource->Update();

	vtkSmartPointer<vtkGlyph3D> glyph = vtkSmartPointer<vtkGlyph3D>::New();
	glyph->SetInputConnection(0, gPoly->GetOutputPort(1));
	glyph->SetInputConnection(1, gSource->GetOutputPort());

	vtkSmartPointer<vtkPolyDataMapper> pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	pdMapper->SetInputConnection(glyph->GetOutputPort());
	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(pdMapper);
	
	//-----
	GLview->AddRepresentationFromInput(g);
	GLview->VertexLabelVisibilityOn();
	GLview->SetVertexLabelArrayName("Name");
	GLview->ColorVerticesOn();
	GLview->SetVertexColorArrayName("Colour");
	GLview->Update();

	vtkSmartPointer<vtkViewTheme> theme = vtkSmartPointer<vtkViewTheme>::New();
	theme->SetPointLookupTable(lookupT);
	GLview->ApplyViewTheme(theme);
	
	GLview->GetRenderer()->AddActor(actor);

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