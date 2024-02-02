//C & C++
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

//VTK
#include <vtkSmartPointer.h>

//VTK Viewer
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkActor.h>

//VTK array/table
#include <vtkTable.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>
#include <vtkIntArray.h>
#include <vtkStringArray.h>

//VTK Graph
#include <vtkPolyDataMapper.h>
#include <vtkGlyph3D.h>
#include <vtkGlyphSource2D.h>
#include <vtkGraphToPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSimple2DLayoutStrategy.h>
#include <vtkGraphLayout.h>
#include <vtkGraphLayoutView.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkDataSetAttributes.h>

//VTK colour
#include <vtkLookupTable.h>
#include <vtkViewTheme.h>

// Global Tables 
//	[Date][Time][Operation][srcAddr][dstAddr][(int)srcPort][(int)dstPort][(int)Weight][EdgeColour]
vtkSmartPointer<vtkTable> networkT = vtkSmartPointer<vtkTable>::New();
//	[(int)PedigreeID][IpPort][Name][Type][VertexColour]
vtkSmartPointer<vtkTable> vertexT = vtkSmartPointer<vtkTable>::New();
vtkSmartPointer<vtkLookupTable> lookupT = vtkSmartPointer<vtkLookupTable>::New();
vtkSmartPointer<vtkMutableDirectedGraph> g = vtkSmartPointer<vtkMutableDirectedGraph>::New();

vtkSmartPointer<vtkIdTypeArray> vertexArray = vtkSmartPointer<vtkIdTypeArray>::New();


//set lookuptable for colouring
void colourTable(){
	
	lookupT->SetNumberOfTableValues(5);
	lookupT->SetTableValue(0, 1.0,1.0,1.0); // white = Regular nodes
	lookupT->SetTableValue(1, 1.0,0.0,1.0); // pink = AFCnetwork nodes
	lookupT->SetTableValue(2, 0.0,1.0,0.0); // green = Build
	lookupT->SetTableValue(3, 0.75,0.75,0.75); // gray = Teardown
	lookupT->SetTableValue(4, 1.0,0.0,0.0,1); // red = Deny
	lookupT->Build();
}//colourTable()

int SetUpTable(){
	
	colourTable();
	
	//Vertex Table
	vtkSmartPointer<vtkIntArray> PedigreeID = vtkSmartPointer<vtkIntArray>::New();
	PedigreeID->SetName("PedigreeID");
	vtkSmartPointer<vtkStringArray> IpPort = vtkSmartPointer<vtkStringArray>::New();
	IpPort->SetName("IpPort");
	vtkSmartPointer<vtkStringArray> Name = vtkSmartPointer<vtkStringArray>::New();
	Name->SetName("Name");
	vtkSmartPointer<vtkStringArray> Type = vtkSmartPointer<vtkStringArray>::New();
	Type->SetName("Type");
	vtkSmartPointer<vtkIntArray> VertexColour = vtkSmartPointer<vtkIntArray>::New();
	VertexColour->SetName("VertexColour");
	vertexT->AddColumn(PedigreeID); // PedigreeID of the vertex on the graph
	vertexT->AddColumn(IpPort); // IP Address or Port number
	vertexT->AddColumn(Name); // Name of IP Address or connected-IP address of the port
	vertexT->AddColumn(Type); // IP Address or Port ('address' OR 'port')
	vertexT->AddColumn(VertexColour); // Colour of the Vertex

	//Network Table
	vtkSmartPointer<vtkStringArray> Date = vtkSmartPointer<vtkStringArray>::New();
	Date->SetName("Date");
	vtkSmartPointer<vtkStringArray> Time = vtkSmartPointer<vtkStringArray>::New();
	Time->SetName("Time");
	vtkSmartPointer<vtkStringArray> Operation = vtkSmartPointer<vtkStringArray>::New();
	Operation->SetName("Operation");
	vtkSmartPointer<vtkStringArray> srcAddr = vtkSmartPointer<vtkStringArray>::New();
	srcAddr->SetName("srcAddr");
	vtkSmartPointer<vtkStringArray> dstAddr = vtkSmartPointer<vtkStringArray>::New();
	dstAddr->SetName("dstAddr");
	vtkSmartPointer<vtkIntArray> srcPort = vtkSmartPointer<vtkIntArray>::New();
	srcPort->SetName("srcPort");
	vtkSmartPointer<vtkIntArray> dstPort = vtkSmartPointer<vtkIntArray>::New();
	dstPort->SetName("dstPort");
	vtkSmartPointer<vtkIntArray> Weight = vtkSmartPointer<vtkIntArray>::New();
	Weight->SetName("Weight");
	vtkSmartPointer<vtkIntArray> EdgeColour = vtkSmartPointer<vtkIntArray>::New();
	EdgeColour->SetName("EdgeColour");
	networkT->AddColumn(Date); // firewall_log date data
	networkT->AddColumn(Time); // firewall_log time data
	networkT->AddColumn(Operation); // firewall_log operation data
	networkT->AddColumn(srcAddr); // firewall_log source IP address
	networkT->AddColumn(dstAddr); // firewall_log destination IP address
	networkT->AddColumn(srcPort); // firewall_log source port number
	networkT->AddColumn(dstPort); // firewall_log destination port number
	networkT->AddColumn(Weight); // number of connection
	networkT->AddColumn(EdgeColour); // number of connection

	return 1;
}

int get_pid(std::string iport){
	int row = 0;
	while(row < vertexT->GetNumberOfRows() && vtkVariant(iport)!= vertexT->GetValueByName(row, "IpPort")){
		row++;
	}
	vtkIdType pid = vertexT->GetValueByName(row, "PedigreeID").ToInt();
	return pid;
}

int vertexExist(std::string iport){

	for(int i=0; i< (int) vertexT->GetNumberOfRows(); i++){
		//vtkSmartPointer<vtkVariantArray> row = vertexT->GetRow(i);

		//if(vtkVariant(iport) == row->GetValue(1)){
		if(vtkVariant(iport) == vertexT->GetValueByName(i, "IpPort")){
			return i;
		}

	}//for(get row)
	return -1;
}

int networkExist(std::vector<std::string> token){
	int matchrow = 0; //found matching table row
	int matchelement = 0;
	
	int i = 0;
	//for: row at the table
	for(; i < (int)networkT->GetNumberOfRows(); i++){

		vtkSmartPointer<vtkVariantArray> row = networkT->GetRow(i);
		
		//compare element from token input & network table row.
		//does not compare the port number for NOW (j<token.size()-2)
		int j = 0;
		for(; j < token.size()-2; j++){
			if(vtkVariant(token[j]) != row->GetValue(j)){
				matchelement = 0;
				break; // get next row to compare
			}else{
				matchelement = 1;
			}
		}//for(get elements)

		// the entire elements in the row match to the token elements.
		if(j == token.size()-2 && matchelement == 1){
			matchrow = 1;
			break;
		}

	}//for(get row)

	//No matching row
	if(matchrow == 0){
		return -1;
	}else{
		return i;
	}
}

//Table format: [PedigreeID][IpPort][Name][Type][VertexColour]
int addVertexT(std::string iport, std::string name, std::string type, std::string AFCnetwork){
	vtkSmartPointer<vtkVariantArray> row = vtkSmartPointer<vtkVariantArray>::New();
	if(AFCnetwork == "AFCnetwork"){
		vtkIdType pid = g->AddVertex();
		row->InsertNextValue(vtkVariant(pid));
		row->InsertNextValue(vtkVariant(iport));
		row->InsertNextValue(vtkVariant(name));
		row->InsertNextValue(vtkVariant(type));
		row->InsertNextValue(vtkVariant(1)); //pink
	}else{
		row->InsertNextValue(vtkVariant(-1));
		row->InsertNextValue(vtkVariant(iport));
		row->InsertNextValue(vtkVariant(name));
		row->InsertNextValue(vtkVariant(type));
		row->InsertNextValue(vtkVariant(0)); // white
	}
	vertexT->InsertNextRow(row);

	//return pid;
	return 1;
}

//Table format: [Date][Time][Operation][srcAddr][dstAddr][srcPort][dstPort][Weight][EdgeColour]
void addNetworkT(std::vector<std::string> info){
	vtkSmartPointer<vtkVariantArray> row = vtkSmartPointer<vtkVariantArray>::New();
	
	for(int i = 0; i < info.size(); i++){
		row->InsertNextValue(vtkVariant(info[i]));
	}
	row->InsertNextValue(1); // weight
	//colour
	if(info[2] == "Built"){
		row->InsertNextValue(vtkVariant(2)); //green
	}else if(info[2] == "Teardown"){
		row->InsertNextValue(vtkVariant(3)); //gray
	}else{//Deny
		row->InsertNextValue(vtkVariant(4)); //red
	}
	networkT->InsertNextRow(row);

	vtkIdType src_pid = 0, dst_pid = 0;
	//check if the vertext of each address(/port) exists
	//source Address
	if(vertexExist(info[3]) < 0){
		//src_pid = addVertexT(info[3], info[3], "IP Address", "other");
		addVertexT(info[3], info[3], "IP Address", "other");
	}/*else{
		src_pid = get_pid(info[3]);
	}*/

	//destination Address
	if(vertexExist(info[4]) < 0){
		//dst_pid = addVertexT(info[4], info[4], "IP Address", "other");
		addVertexT(info[4], info[4], "IP Address", "other");
	}/*else{
		dst_pid = get_pid(info[4]);
	}*/

	//g->AddEdge(src_pid, dst_pid);
}

void AFCnetwork(){

	addVertexT("10.200.150.1", "Firewall", "IP Address", "AFCnetwork");
	addVertexT("172.20.1.1", "Firewall", "IP Address", "AFCnetwork");
	addVertexT("172.20.1.5", "ExternalWebServer", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.1", "Firewall", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.2", "DC1", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.3", "HRDB1", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.4", "SRDB1", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.5", "WEB1", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.6", "EX1", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.7", "FS1", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.14", "DC2", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.16", "Snort IDS", "IP Address", "AFCnetwork");
	addVertexT("192.168.1.50", "Firewall", "IP Address", "AFCnetwork");
	addVertexT("192.168.2.1", "Firewall", "IP Address", "AFCnetwork");
}//AFCnetwork()

int* getTableIndex(vtkTable * table, std::string dt, std::string tm){
	int index[] = {-1,-1};

	if(dt == "dd/mmm/yyyy" && tm == "hh:mm:ss"){ //initial
		index[0] = 0;
		dt = table->GetValueByName(0,"Date").ToString();
		tm = table->GetValueByName(0,"Time").ToString();

		for(int i=1;i<table->GetNumberOfRows();i++){
			std::string tableDate = table->GetValueByName(i,"Date").ToString();
			std::string tableTime = table->GetValueByName(i,"Time").ToString();
			if(dt == tableDate && tm != tableTime){
				break;
			}else{
				index[1] = i;
			}
		}	
		//return index;

	}else{
		
		//find Date index Range
		int dateStartAt = -1;
		int dateEndAt = -1;
		int i = 0;
		std::string tableDate = table->GetValueByName(i,"Date").ToString();
		
		while(i < table->GetNumberOfRows() && tableDate <= dt){
			if(tableDate == dt && dateStartAt < 0){
				dateStartAt = i;
			}
			if(tableDate == dt && dateStartAt >= 0){
				dateEndAt = i;
			}
			i++;
			tableDate = table->GetValueByName(i,"Date").ToString();
		}

		//if found DATE, find Time
		if(dateStartAt >= 0){

			std::string prevTime = "";
			int prevTimeStartAt = -1;
			//find Time index Range
			int j = dateStartAt;
			std::string tableTime = table->GetValueByName(j,"Time").ToString();
			
			while(j <= dateEndAt && tableTime <= tm){

				if(prevTime != tableTime && tableTime < tm){
					prevTime = tableTime;
					prevTimeStartAt = j;
				}

				if(tableTime == tm && index[0] < 0){
					index[0] = j;
				}
				if(tableTime == tm && index[0] >= 0){
					index[1] = j;
				}
				j++;
				tableTime = table->GetValueByName(j,"Time").ToString();
			}

			//i no Time, get previous time range (exist)
			if(index[0] < 0 && index[1] < 0 && prevTimeStartAt >= 0){
				std::cout<<"No Available data at " << tm << std::endl;
				std::cout<<"Data at " << prevTime << std::endl;
				
				index[0] = prevTimeStartAt;
				
				int k = prevTimeStartAt;
				while(k <= dateEndAt && tableTime < tm){
					k++;
					tableTime = table->GetValueByName(k,"Time").ToString();
				}

				index[1] = k-1;
			}

		}else{
			std::cout<<"No Available data on " << dt << std::endl;
		}

	}
	return index; 
}

int drawGraph(int beginAt, int endAt){
	
	int i = beginAt;
	
	for(; i < endAt+1 ; i++){
		
		//get address/port#.
		std::string src = networkT->GetValueByName(i, "srcAddr").ToString();
		std::string dst = networkT->GetValueByName(i, "dstAddr").ToString();

		//vertexTable index
		int srcV = vertexExist(src);
		int dstV = vertexExist(dst);
		int srcpid = -1, dstpid = -1;

		if(vertexT->GetValueByName(srcV,"PedigreeID").ToInt() == -1){
			srcpid = g->AddVertex();
			vertexT->SetValueByName(srcV, "PedigreeID", srcpid);
			vertexArray->InsertNextValue(srcpid);
		}else{
			srcpid = get_pid(src);
		}

		if(vertexT->GetValueByName(dstV,"PedigreeID").ToInt() == -1){
			dstpid = g->AddVertex();
			vertexT->SetValueByName(dstV, "PedigreeID", dstpid);
			vertexArray->InsertNextValue(dstpid);
		}else{
			dstpid = get_pid(dst);
		}

		g->AddEdge(srcpid, dstpid);
		
	}

	return 1;
}

int FirewallLogData(char* logfile){
	
	std::ifstream f(logfile);
	std::string line;

	while(std::getline(f,line) && line.length() > 0){
		try{
 
			//token
			std::stringstream ss(line);
			std::string tempstr;
			std::vector<std::string> token;
			/* 	[date/time]	[syslog]		[operation]		[msg code]		[protocol]
				[srcIP]		[dstIP]			[src_hostname]	[dst_hostname]	[src port]
				[dst port]	[dst service]	[direction]		[connect_built]	[connect_torndown] */
			while(std::getline(ss,tempstr,',')){
				token.push_back(tempstr);
			}
			
			//remove un-necessary elements
			token.erase(token.begin()+14); //connect_torndown
			token.erase(token.begin()+13); //connect_build
			token.erase(token.begin()+12); //direction
			token.erase(token.begin()+11); //dst service
			token.erase(token.begin()+8); //dst_hostname
			token.erase(token.begin()+7); //src_hostname
			token.erase(token.begin()+4); //protocol
			token.erase(token.begin()+3); //msg_code
			token.erase(token.begin()+1); //syslog

			std::string datetime = token[0];
			std::size_t spacefound = datetime.find_first_of(" ",0);
			if(spacefound != std::string::npos){
				//split date & time
				std::string ldate, ltime;
				ldate = datetime.substr(0,spacefound);
				ltime = datetime.substr(spacefound+1, datetime.length());
				
				token.erase(token.begin());
				token.insert(token.begin(),ltime);
				token.insert(token.begin(),ldate);
				//token = [date][time][op][srcAddr][dstAddr][srcPort][dstPort]
								
				if((int)networkT->GetNumberOfRows() == 0){
					addNetworkT(token);
				}else{
					int row = networkExist(token);
					if(row < 0){
						addNetworkT(token);
					}else{
						int atRow = row;
						int weightCol = 7;
						int w = networkT->GetValue(atRow, weightCol).ToInt();
						networkT->SetValue(atRow, weightCol, w+1); // increase the weight by one
					}
				}

			}

		}catch(std::exception e){
			std::cout << "Try-Catch Exception called at FirewallLogData()" << std::endl;
			return 0;
		}
	}
	return 1;
}//void FirewallLogData()

int GraphNetworkFlow(std::string dateInfo, std::string timeInfo){
	/*std::string dt,tm;
	std::size_t spacefound = datetime.find_first_of(" ",0);
	
	// split date & time
	if(spacefound != std::string::npos){ // date & time
		dt = datetime.substr(0,spacefound);
		tm = datetime.substr(spacefound+1, datetime.length());
		if(dt.find("/",0) == std::string::npos || tm.find(":",0) == std::string::npos){
			std::cout<< "stdin Format Error. \nUsage: <dd/Mmm(in Alphabetic)/yyyy [optional: hh/mm/ss]>";
			std::cout<<"\ne.g. 13/Apr/2011 08:52:53 or 13/Apr/2011" << std::endl;
			return 0;
		}
	}else if(datetime.find("/",0) != std::string::npos && datetime.find(":",0) == std::string::npos){ // only date

		dt = datetime;
		tm = "hh:mm:ss";
	}else{ // no date
		std::cout<< "stdin Format Error. \nUsage: <dd/Mmm(in Alphabetic)/yyyy [optional: hh/mm/ss]>";
		std::cout<<"\ne.g. 13/Apr/2011 08:52:53 or 13/Apr/2011" << std::endl;
		return 0;	
	}*/

	int * index = getTableIndex(networkT, dateInfo, timeInfo);
	int beginAt=*index, endAt=*(index+1);
	if(beginAt >=0 && endAt >= 0){ 
		drawGraph(beginAt, endAt); 
	}else{
		std::cout<< "No data available at (GraphNetworkFlow())" << std::endl;
		return 0;
	}
	return 1;

}//GraphNetworkFlow

int Display(){
	vtkSmartPointer<vtkGraphLayoutView> GLview = vtkSmartPointer<vtkGraphLayoutView>::New();
	
	//-----
	vtkSmartPointer<vtkGraphLayout> GraphLayout = vtkSmartPointer<vtkGraphLayout>::New();
	vtkSmartPointer<vtkSimple2DLayoutStrategy> strategy = vtkSmartPointer<vtkSimple2DLayoutStrategy>::New();
	GraphLayout->SetInputData(g);
	GraphLayout->SetLayoutStrategy(strategy);

	GLview->SetLayoutStrategyToSimple2D(); // SetLayoutStrategyToPassThrough();
	GLview->SetEdgeLayoutStrategyToPassThrough();
	GLview->AddRepresentationFromInputConnection(GraphLayout->GetOutputPort());

	vtkSmartPointer<vtkGraphToPolyData> GraphToPolyData = vtkSmartPointer<vtkGraphToPolyData>::New();
	GraphToPolyData->SetInputConnection(GraphLayout->GetOutputPort());
	GraphToPolyData->EdgeGlyphOutputOn();
	GraphToPolyData->SetEdgeGlyphPosition(1);

	vtkSmartPointer<vtkGlyphSource2D> GlyphSource2D = vtkSmartPointer<vtkGlyphSource2D>::New();
	GlyphSource2D->SetGlyphTypeToEdgeArrow();
	GlyphSource2D->SetScale(0.008);
	GlyphSource2D->Update();

	vtkSmartPointer<vtkGlyph3D> Glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
	Glyph3D->SetInputConnection(0, GraphToPolyData->GetOutputPort(1));
	Glyph3D->SetInputConnection(1, GlyphSource2D->GetOutputPort());

	vtkSmartPointer<vtkPolyDataMapper> PolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	PolyDataMapper->SetInputConnection(Glyph3D->GetOutputPort());
	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(PolyDataMapper);
	
	//-----
	GLview->AddRepresentationFromInput(g);
	GLview->VertexLabelVisibilityOn();
	GLview->SetVertexLabelArrayName("Name");
	//GLview->ColorVerticesOn();
	//GLview->SetVertexColorArrayName("VertexColour");
	GLview->EdgeLabelVisibilityOn();
	GLview->SetEdgeLabelArrayName("Weight");
	//GLview->ColorEdgesOn();
	//GLview->SetEdgeColorArrayName("EdgeColour");

	/*std::cout<< "get vertex color array: " << GLview->GetVertexColorArrayName() << std::endl;
	std::cout<< "get edge color array: " << GLview->GetEdgeColorArrayName() << std::endl;
	for(int i = 0; i < vertexT->GetNumberOfRows(); i++){
		std::cout<< vertexT->GetValueByName(i,GLview->GetVertexColorArrayName())<<std::endl;
	}
	for(int i = 0; i < networkT->GetNumberOfRows(); i++){
		std::cout<< networkT->GetValueByName(i,GLview->GetEdgeColorArrayName())<<std::endl;
	}*/
	GLview->Update();

	vtkSmartPointer<vtkViewTheme> ViewTheme = vtkSmartPointer<vtkViewTheme>::New();
	ViewTheme->SetPointLookupTable(lookupT);
	GLview->ApplyViewTheme(ViewTheme);
	
	GLview->GetRenderer()->AddActor(actor);

	GLview->ResetCamera();
	//GLview->GetInteractor()->Initialize();
	GLview->Render();
	GLview->GetInteractor()->Start();
}

int main(int argc, char* argv[]){
	
	if(argc != 2){
		std::cout << "Usage: ./networkop_afc <firewall_log>" << std::endl;
		return 0;
	}

	SetUpTable();

	//set All Freight Corporation Netowrk Nodes
	int numAFCNetwork = 14;
	AFCnetwork();

	char* firewall_log = argv[1];
	FirewallLogData(firewall_log);

	for(int i = 0; i< (int)networkT->GetNumberOfRows(); i++){
		vtkSmartPointer<vtkVariantArray> row = networkT->GetRow(i);

		for(int j =0; j<row->GetNumberOfValues(); j++){
			std::cout << row->GetValue(j) << " || ";
		}
		std::cout<<std::endl;
	}

	GraphNetworkFlow("dd/mmm/yyyy","hh:mm:ss"); //input format: dd/Apr/2011 hh:mm:ss (24hr)
	
	g->GetVertexData()->AddArray(vertexT->GetColumnByName ("Name"));
	
	std::cout<< "Enter Date/Time of the data to display. Format: dd/mmm/yyyy [optional: hh:mm:ss]"<<std::endl;
	std::cout<< "e.g. 13/Apr/2011 08:52:53 or 13/Apr/2011"<<std::endl;
	Display();

	std::string keyboard;
	getline (std::cin,keyboard);
	while(keyboard != "q" || keyboard != "exit"){
		
		std::string inputDate,inputTime;
		std::size_t spacefound = keyboard.find_first_of(" ",0);
	
		// split date & time
		if(spacefound != std::string::npos){ // date & time
			inputDate = keyboard.substr(0,spacefound);
			inputTime = keyboard.substr(spacefound+1, keyboard.length());
			if(inputDate.find("/",0) != std::string::npos || inputTime.find(":",0) != std::string::npos){
				g->RemoveVertices(vertexArray);
				for(int i = numAFCNetwork;i<vertexT->GetNumberOfRows();i++){
					vertexT->SetValueByName(i,"PedigreeID",-1);
				}
		
				GraphNetworkFlow(inputDate, inputTime);
				g->GetVertexData()->AddArray(vertexT->GetColumnByName ("Name"));
				Display();

			}else{
				std::cout<< "stdin Format Error. \nUsage: <dd/Mmm(in Alphabetic)/yyyy [optional: hh/mm/ss]>";
				std::cout<<"\ne.g. 13/Apr/2011 08:52:53 or 13/Apr/2011" << std::endl;
			}
		}else if(keyboard.find("/",0) != std::string::npos && keyboard.find(":",0) == std::string::npos){ // only date
			g->RemoveVertices(vertexArray);
			for(int i = numAFCNetwork;i<vertexT->GetNumberOfRows();i++){
				vertexT->SetValueByName(i,"PedigreeID",-1);
			}

			inputDate = keyboard;
			inputTime = "hh:mm:ss";
			GraphNetworkFlow(inputDate, inputTime);
			g->GetVertexData()->AddArray(vertexT->GetColumnByName ("Name"));
			Display();
		}else{ // no date
			std::cout<< "stdin Format Error. \nUsage: <dd/Mmm(in Alphabetic)/yyyy [optional: hh/mm/ss]>";
			std::cout<<"\ne.g. 13/Apr/2011 08:52:53 or 13/Apr/2011" << std::endl;
		}
		getline (std::cin,keyboard);
	}

	/*//Vertex/Edge label & colouring
	g->GetVertexData()->AddArray(vertexT->GetColumnByName ("Name"));
	//g->GetVertexData()->AddArray(vertexT->GetColumnByName ("VertexColour"));
	//g->GetEdgeData()->AddArray(networkT->GetColumnByName("Weight"));
	//g->GetEdgeData()->AddArray(networkT->GetColumnByName("EdgeColour"));

	Display();*/

	//createTrackbar("Date/Time Line", "CNO Situation Awareness", &usertime, usertime+3600, network);
  	return 0;
}
