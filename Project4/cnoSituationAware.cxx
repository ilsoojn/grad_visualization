/*
 * CEG 7560 - Visualization & Image process
 * Final Project
 * More detailed information about the program is in the README file.
 */


//C & C++
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <time.h>

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
#include <vtkIdTypeArray.h>

//VTK Chart Graph
#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>

//VTK Graph
#include <vtkPolyDataMapper.h>
#include <vtkGlyph3D.h>
#include <vtkGlyphSource2D.h>
#include <vtkGraphToPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSimple2DLayoutStrategy.h>
#include <vtkGraphLayout.h>
#include <vtkGraphLayoutView.h>
#include <vtkMutableUndirectedGraph.h>
#include <vtkDataSetAttributes.h>

//VTK colour
#include <vtkLookupTable.h>
#include <vtkViewTheme.h>

//VTK Slider
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkSliderWidget.h>
#include <vtkSliderRepresentation3D.h>

// Global Tables
//	[UnixTimestamp][Operation][srcAddr][dstAddr][(int)srcPort][(int)dstPort][(int)Weight][EdgeColour]
vtkSmartPointer<vtkTable> networkT = vtkSmartPointer<vtkTable>::New();
//	[(int)PedigreeID][IpPort][Name][Type][VertexColour]
vtkSmartPointer<vtkTable> vertexT = vtkSmartPointer<vtkTable>::New();
//[timeline][FS1][EX1][WEB1][SRDB1][HRDB1][DC1][DC2][Count]
vtkSmartPointer<vtkTable> plotT = vtkSmartPointer<vtkTable>::New();

vtkSmartPointer<vtkLookupTable> lookupT = vtkSmartPointer<vtkLookupTable>::New();
vtkSmartPointer<vtkMutableUndirectedGraph> g = vtkSmartPointer<vtkMutableUndirectedGraph>::New();

// Graph Display Character Array
vtkSmartPointer<vtkIdTypeArray> vertexArray = vtkSmartPointer<vtkIdTypeArray>::New(); //Vertices in Undirect Graph
vtkSmartPointer<vtkStringArray> vertexName = vtkSmartPointer<vtkStringArray>::New(); //Vertices Name Label for the Graph
vtkSmartPointer<vtkIntArray> vertexColour = vtkSmartPointer<vtkIntArray>::New(); // Vertices Colour
vtkSmartPointer<vtkStringArray> edgeWeight = vtkSmartPointer<vtkStringArray>::New(); // Edge Weight Label Value for the Graph
vtkSmartPointer<vtkIntArray> edgeColour = vtkSmartPointer<vtkIntArray>::New(); // Edge Colour on the Graph

std::string displayDATE = ""; //Current Display Date/Time on the Undirect Graph

/*
 * Conver String to Integer
 * Parameter: String number/digit
 * Return: Integer value
 */
int toInt(std::string str){
	int integer;
	std::stringstream ss;
	ss << str;
	ss >> integer;
	return integer;
} //toInt()

/*
 * Conver Integer to String
 * Parameter: integer number
 * Return: String value of the integer
 */
std::string toString(int integer){
	std::stringstream ss;
	ss << integer;
	return ss.str();
} //toString()

// Convert Date & Time String into Unix Time Stamp & store it into the Table Column
void getTableIndexUTS(vtkTable * T, int uts, std::vector<int> * index);
// Create Vertices & Edges and draw an Undirect Graph with them.
int drawGraph(int start, int stop); 

/*
 * VTK CallBack function for the Slider
 * For the current slider's position,
 * 		Get the unix Time; then, get the range of index from the networkTable to display
 *  	Draw an Undirect Graph with the new networkTable information.
 * (* Write out the number of vertices in the graph on the console)
 */
class vtkSliderCallback: public vtkCommand{
public:
	static vtkSliderCallback * New() {
    	return new vtkSliderCallback;
  	}

  	virtual void Execute(vtkObject *caller, unsigned long, void*){
  		vtkSliderWidget *sliderWidget = reinterpret_cast<vtkSliderWidget*>(caller);
        double value = static_cast<vtkSliderRepresentation *>(sliderWidget->GetRepresentation())->GetValue();
  		int utime = (int)(value * 10);
  		utime += toInt(displayDATE);
  		std::vector<int> index;
  		getTableIndexUTS(networkT, utime, &index);
  		std::cout << "-------------------------------------" << std::endl;
  		std::cout << utime << " ("<<index[0] << ", " << index[1]<<")" << std::endl;
  		drawGraph(index[0], index[1]);
  		
  		std::cout<<"Number of Vertices in the Graph g: " << g->GetNumberOfVertices() << std::endl;
  		
  	}
  	vtkSliderCallback():MutableUndirectedGraph(0) {}
  	vtkSmartPointer<vtkMutableUndirectedGraph> MutableUndirectedGraph;
}; //vtkSliderCallback class

//set lookuptable for colouring
void colourTable(){

	lookupT->SetNumberOfTableValues(5);
	lookupT->SetTableValue(0, 1.0,1.0,1.0); // white = Regular nodes
	lookupT->SetTableValue(1, 1.0,0.0,1.0); // pink = AFCnetwork nodes
	lookupT->SetTableValue(2, 0.0,1.0,0.0); // green = Build/Teardown
	lookupT->SetTableValue(3, 1.0,0.0,0.0); // red = Deny
	lookupT->Build();
}// colourTable()

/*
 * Convert date & time string to UnixTimestamp.
 * Input: string Date and Time in following format: dd/Mth/yyyy hh:mm:ss 
 *		where 'Mth' is the first three spelling of the month
 * Return: UnixTimestamp varaible.
 */
time_t toUnixTimestamp(std::string dtime){

	time_t rawtime;
	time(&rawtime);
	tm * unixtime = localtime(&rawtime);

	try{

		/*********************************************************/
		/*** parse date & time information into integer array. ***/
		/*********************************************************/
		int numMonths = 12;
		std::string Months[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
		int datetime[6]={}; //[dd][mm][yyyy][hh][mm][ss]
		int i=0;

		//DATE
		while(i<2){

			std::size_t slash = dtime.find("/",0);
			std::stringstream ss(dtime.substr(0,slash));
			ss >> datetime[i];

			//In the case where given input month value is not in Number
			//Get the corresponding integer to represent the name of the month
			//	using 'Months[12]' array
			if(ss.fail()){
				std::string month = dtime.substr(0,slash);
				for(int m = 0; m < numMonths; m++){
					if(month == Months[m]){
						datetime[i]=m+1;
						break;
					}
				}
			}
			dtime = dtime.substr(slash+1,dtime.length());
			i++;
		}
		//Remove the  white-space between the Date & Time and get the first time value (Hour)
		std::size_t space = dtime.find(" ",0);
		std::stringstream ss(dtime.substr(0,space));
		ss >> datetime[i];
		dtime = dtime.substr(space+1,dtime.length());
		i++;

		//TIME
		while(i<6){
			std::size_t colon = dtime.find(":",0);
			std::stringstream ss(dtime.substr(0,colon));
			ss >> datetime[i];
			dtime = dtime.substr(colon+1,dtime.length());
			i++;
		}

		/********************************************************/
		/*** Set & Convert parsed date/time to Unix Timestamp ***/
		/********************************************************/

		unixtime->tm_mday = datetime[0]; // day
		unixtime->tm_mon = datetime[1] - 1; // month
		unixtime->tm_year = datetime[2] - 1900; // year
		unixtime->tm_hour = datetime[3]; // hour
		unixtime->tm_min = datetime[4]; // minute
		unixtime->tm_sec = datetime[5]; // second

	}catch(int e){ return time_t(-1); }

	return mktime(unixtime); // return Unix time value
}// toUnixTimestamp()

/*
 * Basic SetUp for the global Arrays & Tables
 * Return: 1 (to notice the function is succesfully worked.)
 */
int SetUpTable(){

	//Create LookUp Table for Colouring the Vertices & the Edges on the graph
	colourTable();

	/********************************************************/
	/************ Arrays : Set Name of each Array ***********/
	/********************************************************/

	vertexName->SetName("vertexName");
	vertexColour->SetName("vertexColour");
	edgeWeight->SetName("edgeWeight");
	edgeColour->SetName("edgeColour");

	/********************************************************/
	/********************* Global TABLEs ********************/
	/************ Create Arrays and set the name ************/
	/** Each Array will be the column of the related Table **/
	/********************************************************/

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
	vtkSmartPointer<vtkStringArray> UnixTimestamp = vtkSmartPointer<vtkStringArray>::New();
	UnixTimestamp->SetName("UnixTimestamp");
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
	
	networkT->AddColumn(UnixTimestamp); // firewall_log date/time in Unix Timestamp
	networkT->AddColumn(Operation); // firewall_log operation data
	networkT->AddColumn(srcAddr); // firewall_log source IP address
	networkT->AddColumn(dstAddr); // firewall_log destination IP address
	networkT->AddColumn(srcPort); // firewall_log source port number
	networkT->AddColumn(dstPort); // firewall_log destination port number
	networkT->AddColumn(Weight); // number of connection
	networkT->AddColumn(EdgeColour); // number of connection

	//Plot Table
	vtkSmartPointer<vtkIntArray> timeline = vtkSmartPointer<vtkIntArray>::New();
	timeline->SetName("timeline");
	vtkSmartPointer<vtkIntArray> FS1 = vtkSmartPointer<vtkIntArray>::New();
	FS1->SetName("FS1");
	vtkSmartPointer<vtkIntArray> EX1 = vtkSmartPointer<vtkIntArray>::New();
	EX1->SetName("EX1");
	vtkSmartPointer<vtkIntArray> WEB1 = vtkSmartPointer<vtkIntArray>::New();
	WEB1->SetName("WEB1");
	vtkSmartPointer<vtkIntArray> SRDB1 = vtkSmartPointer<vtkIntArray>::New();
	SRDB1->SetName("SRDB1");
	vtkSmartPointer<vtkIntArray> HRDB1 = vtkSmartPointer<vtkIntArray>::New();
	HRDB1->SetName("HRDB1");
	vtkSmartPointer<vtkIntArray> DC1 = vtkSmartPointer<vtkIntArray>::New();
	DC1->SetName("DC1");
	vtkSmartPointer<vtkIntArray> DC2 = vtkSmartPointer<vtkIntArray>::New();
	DC2->SetName("DC2");
	vtkSmartPointer<vtkIntArray> Count = vtkSmartPointer<vtkIntArray>::New();
	Count->SetName("Count");

	plotT->AddColumn(timeline);
	plotT->AddColumn(FS1);
	plotT->AddColumn(EX1);
	plotT->AddColumn(WEB1);
	plotT->AddColumn(SRDB1);
	plotT->AddColumn(HRDB1);
	plotT->AddColumn(DC1);
	plotT->AddColumn(DC2);
	plotT->AddColumn(Count);

	return 1;
}// SetUpTable()

/*
 * For the given vertex's IP/port,
 *		Find the matching PedigreeID of the vertex.
 * Input: Vertex's IP/port number in string.
 * Return: Pedigree ID of the vertex
 */
int get_pid(std::string iport){
	int row = 0;
	
	// Find the matching IP/port's row index from the vertexTable
	while(row < vertexT->GetNumberOfRows() && vtkVariant(iport)!= vertexT->GetValueByName(row, "IpPort")){
		row++;
	}

	// Get the PedigreeID of the matching vertex's IP/port from the vertex Table.
	vtkIdType pid = vertexT->GetValueByName(row, "PedigreeID").ToInt();
	
	return pid;
}// get_pid()

/*
 * To see if the vertex's IP/port is alread created
 * 		Read & compare each row's 'IpPort' of vertex Table
 * Input: IP/port number in string type
 *		Find the matching PedigreeID of the vertex.
 * Return: Row index on the vertex Table which has information on the IP/port.
 */
int vertexExist(std::string iport){
	// Go through each row in the vertex Table
	// to find the given IP/port 
	for(int i=0; i< (int) vertexT->GetNumberOfRows(); i++){
		
		if(vtkVariant(iport) == vertexT->GetValueByName(i, "IpPort")){
			return i;
		}

	}//for(get row)
	return -1;
}// vertexExist()

/*
 * To see if the same Firewall-Log's network data exist in the Table (to prevent Duplication of same data)
 * 		Go through each rows in the network Table & each elements in the row;
 * Input: Row information for Network Table
 * Return:	If every elements in the row matches, return the row index of the network Table
 *			Else -1
 */
int networkExist(std::vector<std::string> token){
	int matchrow = 0; //found matching table row
	int matchelement = 0;

	int r = 0;
	//for: each row in the table
	for(; r < (int)networkT->GetNumberOfRows(); r++){

		vtkSmartPointer<vtkVariantArray> row = networkT->GetRow(r);

		//compare element from token input & network table row.
		//does not compare the port number for NOW (c<token.size()-2)
		int c = 0;
		for(; c < token.size()-2; c++){
			if(vtkVariant(token[c]) != row->GetValue(c)){
				matchelement = 0;
				break; // get next row to compare
			}else{
				matchelement = 1;
			}
		}//for(get elements)

		// the entire elements in the row match to the token elements.
		if(c == token.size()-2 && matchelement == 1){
			matchrow = 1;
			break;
		}

	}//for(get row)

	//No matching row
	if(matchrow == 0){
		return -1;
	}else{
		return r;
	}
}// networkExist()

/*
 * Add a row in the vertex Table in following Table format:
 *		[PedigreeID][Name][IpPort][Type][VertexColour]
 * Input: data for the table (Name, IP Address/Port number, Type, Colour for the vertex)
 * If the vertices are from All Freight Corporation System,
 *		Create Vertex on the graph and set the vertex colour as pink
 *		The return value will be the vertex's pedigree ID.
 * Else Do not create the vertex on the graph, but insert the vertex data into the Table
 *		Not creating vertex, set pedgree ID data as -1
 * Return: pedigreeID (either from the graph or -1)
 */
int addVertexT(std::string iport, std::string name, std::string type, std::string AFCnetwork){
	vtkIdType pid = -1;
	vtkSmartPointer<vtkVariantArray> row = vtkSmartPointer<vtkVariantArray>::New();
	
	// Fixed All Freight Corporation Network
	if(AFCnetwork == "AFCnetwork"){
		
		pid = g->AddVertex();
		vertexName->InsertNextValue(name);
		vertexColour->InsertNextValue(1);
		
		row->InsertNextValue(vtkVariant(pid));
		row->InsertNextValue(vtkVariant(iport));
		row->InsertNextValue(vtkVariant(name));
		row->InsertNextValue(vtkVariant(type));
		row->InsertNextValue(vtkVariant(1)); //pink
	}else{
		// Flexible IPAddresses / Port number
		row->InsertNextValue(vtkVariant(-1));
		row->InsertNextValue(vtkVariant(iport));
		row->InsertNextValue(vtkVariant(name));
		row->InsertNextValue(vtkVariant(type));
		row->InsertNextValue(vtkVariant(0)); // white
	}
	vertexT->InsertNextRow(row);
	
	return pid;
}// addVertexT()

/*
 * Add a new row in the network Table in following Table format:
 *		[UnixTimestamp][Operation][srcAddr][dstAddr][srcPort][dstPort][Weight][EdgeColour]
 * 		Weight will be always set to 1 since it just created the row
 * 		Edge colour will be depend on the operation of the data from 'Firewall_log';
 * If vertex for each source and destination Address doesn't exist, create new vertex information in the vertex Table.
 * Input: vector of data to fill the table EXCEPT the weight & Edge's colour for the graph
 */
void addNetworkT(std::vector<std::string> info){
	
	vtkSmartPointer<vtkVariantArray> row = vtkSmartPointer<vtkVariantArray>::New();
	
	// copy given information into the row.				
	for(int i = 0; i < info.size(); i++){
		row->InsertNextValue(vtkVariant(info[i]));
	}
					
	row->InsertNextValue(1); // weight
	
	//colour
	if(info[1] == "Built"){
		row->InsertNextValue(vtkVariant(2)); //green
	}else if(info[1] == "Teardown"){
		row->InsertNextValue(vtkVariant(3)); //gray
	}else{//Deny
		row->InsertNextValue(vtkVariant(4)); //red
	}

	networkT->InsertNextRow(row); // add the new row into the network Table

	// Check if the vertext of each IP Address(/port) exists
	// If NOT, then add new vertex information with the IPAddress(or port)
	if(vertexExist(info[2]) < 0){ // source Address
		addVertexT(info[2], info[2], "IP Address", "other");
	}

	if(vertexExist(info[3]) < 0){ // destination Address
		addVertexT(info[3], info[3], "IP Address", "other");
	}
}// addNetworkT()

/*
 * Hard code the fixed All Freight Corporation's Network System
 * 		Create vertex and add vertex data into the vertex Table.
 * 		Then, create link( =edge) of the graph to represent the system's network connection.
 * In addition, set the weight label for the link to be 'AFC Network' since there aren't weight on these links
 * 		+ set these network link's colours to be pink.
 */
void AFCnetwork(){

	int CiscoFW = addVertexT("xxx.xxx.xxx.xxx", "CiscoFirewall", "CiscoFirewall", "AFCnetwork");
	int InternetFW = addVertexT("10.200.150.1", "InternetFirewall", "Firewall", "AFCnetwork");
	int ExtWebServerFW = addVertexT("172.20.1.1", "ExtWebFirewall", "Firewall", "AFCnetwork");
	int ExtWebServer = addVertexT("172.20.1.5", "ExternalWebServer", "Server", "AFCnetwork");
	int vlan10FW = addVertexT("192.168.1.1", "vlan10Firewall", "Firewall", "AFCnetwork");
	int DC1 = addVertexT("192.168.1.2", "DC1", "Server", "AFCnetwork");
	int HRDB1 = addVertexT("192.168.1.3", "HRDB1", "Server", "AFCnetwork");
	int SRDB1 = addVertexT("192.168.1.4", "SRDB1", "Server", "AFCnetwork");
	int WEB1 = addVertexT("192.168.1.5", "WEB1", "Server", "AFCnetwork");
	int EX1 = addVertexT("192.168.1.6", "EX1", "Server", "AFCnetwork");
	int FS1 = addVertexT("192.168.1.7", "FS1", "Server", "AFCnetwork");
	int DC2 = addVertexT("192.168.1.14", "DC2", "Server", "AFCnetwork");
	int IDS = addVertexT("192.168.1.16", "Snort IDS", "IDS", "AFCnetwork");
	int Firewall_Log = addVertexT("192.168.1.50", "LogFirewall", "Firewalls", "AFCnetwork");
	int vlan20FW = addVertexT("192.168.2.1", "vlan20Firewall", "Firewall", "AFCnetwork");

	// Cisco ASA5510 & Cisco 3750E Switch
	g->AddEdge(InternetFW, CiscoFW);
	g->AddEdge(ExtWebServerFW, CiscoFW);
	g->AddEdge(vlan10FW, CiscoFW);
	g->AddEdge(vlan20FW, CiscoFW);
	g->AddEdge(InternetFW, CiscoFW);
	g->AddEdge(IDS, CiscoFW);

	//from External Web Server Link
	g->AddEdge(ExtWebServer, ExtWebServerFW);
	
	//Servers-Firewall_Log Link
	g->AddEdge(DC1, Firewall_Log);
	g->AddEdge(HRDB1, Firewall_Log);
	g->AddEdge(SRDB1, Firewall_Log);
	g->AddEdge(WEB1, Firewall_Log);
	g->AddEdge(WEB1, Firewall_Log);
	g->AddEdge(EX1, Firewall_Log);
	g->AddEdge(FS1, Firewall_Log);
	g->AddEdge(DC2, Firewall_Log);

	//Servers-SnortIDS Link
	g->AddEdge(DC1, IDS);
	g->AddEdge(HRDB1, IDS);
	g->AddEdge(SRDB1, IDS);
	g->AddEdge(WEB1, IDS);
	g->AddEdge(WEB1, IDS);
	g->AddEdge(EX1, IDS);
	g->AddEdge(FS1, IDS);
	g->AddEdge(DC2, IDS);

	for(int i=0;i<g->GetNumberOfEdges();i++){
		edgeColour->InsertNextValue(1);
		edgeWeight->InsertNextValue("AFC Network");
	}
}//AFCnetwork()

/*
 * Add a new row in the plot Table in following Table format:
 *		[timeline][FS1][EX1][WEB1][SRDB1][HRDB1][DC1][DC2][Count]
 * For each Date/Time (unixtime), count the number of network traffic for each server
 * This plot table will be used to draw a XY Plot line-graph.
 */
void plotTable(){
	std::string udate = "";
	int FS1 = 0, EX1 = 0, WEB1 = 0, SRDB1 = 0, HRDB1 = 0, DC1 = 0, DC2 = 0, count = 0;
	
	for(int r = 0; r < networkT->GetNumberOfRows(); r++){

		// If new date/time
		// 		Create new row for the previosu date/time data and insert counts in the order.
		// 		Add into the plot table to graph xyplot.
		if(udate != networkT->GetValueByName(r,"UnixTimestamp").ToString()){
			vtkSmartPointer<vtkVariantArray> row = vtkSmartPointer<vtkVariantArray>::New();
			if(count > 0){
				row->InsertNextValue(vtkVariant(udate));
				row->InsertNextValue(vtkVariant(FS1));
				row->InsertNextValue(vtkVariant(EX1));
				row->InsertNextValue(vtkVariant(WEB1));
				row->InsertNextValue(vtkVariant(SRDB1));
				row->InsertNextValue(vtkVariant(HRDB1));
				row->InsertNextValue(vtkVariant(DC1));
				row->InsertNextValue(vtkVariant(DC2));
				row->InsertNextValue(vtkVariant(count));
				plotT->InsertNextRow(row);
			}

			//current_date/time = new date/time
			udate = networkT->GetValueByName(r,"UnixTimestamp").ToString();
			// Reset counts
			count = 1;
			FS1 = 0; EX1 = 0; WEB1 = 0; SRDB1 = 0; HRDB1 = 0; DC1 = 0; DC2 = 0; count = 0;
			
			// increment the number of matching network connectivity

			std::string src = networkT->GetValueByName(r,"srcAddr").ToString(); // Source Address
			if(src == "192.168.1.7"){FS1++;}
			if(src == "192.168.1.6"){EX1++;}
			if(src == "192.168.1.5"){WEB1++;}
			if(src == "192.168.1.4"){SRDB1++;}
			if(src == "192.168.1.3"){HRDB1++;}
			if(src == "192.168.1.2"){DC1++;}
			if(src == "192.168.1.14"){DC2++;}

			std::string dst = networkT->GetValueByName(r,"dstAddr").ToString(); // Destination Address
			if(dst == "192.168.1.7"){FS1++;}
			if(dst == "192.168.1.6"){EX1++;}
			if(dst == "192.168.1.5"){WEB1++;}
			if(dst == "192.168.1.4"){SRDB1++;}
			if(dst == "192.168.1.3"){HRDB1++;}
			if(dst == "192.168.1.2"){DC1++;}
			if(dst == "192.168.1.14"){DC2++;}
			
		}else{

			// Increment the number of matching network Conectivity for current_date/time

			std::string src = networkT->GetValueByName(r,"srcAddr").ToString(); // Source Address
			if(src == "192.168.1.7"){FS1++;}
			if(src == "192.168.1.6"){EX1++;}
			if(src == "192.168.1.5"){WEB1++;}
			if(src == "192.168.1.4"){SRDB1++;}
			if(src == "192.168.1.3"){HRDB1++;}
			if(src == "192.168.1.2"){DC1++;}
			if(src == "192.168.1.14"){DC2++;}

			std::string dst = networkT->GetValueByName(r,"dstAddr").ToString(); // Destination Address
			if(dst == "192.168.1.7"){FS1++;}
			if(dst == "192.168.1.6"){EX1++;}
			if(dst == "192.168.1.5"){WEB1++;}
			if(dst == "192.168.1.4"){SRDB1++;}
			if(dst == "192.168.1.3"){HRDB1++;}
			if(dst == "192.168.1.2"){DC1++;}
			if(dst == "192.168.1.14"){DC2++;}
			count++;
		}
	}
}// plotTable()

/*
 * Get the index range (start row index & end row index) of matching date/time (int unixts) from the network table
 * IF there is no matching date/time data in the table, get the row index range for the preivious time period (one before the given time)
 * Input: Table to lookup the date/time, time/date in unix timestamp, index vector pointer
 * (*index vector pointer would be output of the function)
 */
void getTableIndexUTS(vtkTable * table, int unixts, std::vector<int> * index){
	
	int starttime = -1, endtime = -1; // initial index range

	int i=0;
	int prevTime = -1; // Date/Time which is one before the given date/time info
	int tableDate = table->GetValueByName(i,"UnixTimestamp").ToInt(); // date/time unix-timestamp type variable from the table
	
	//find the matching date/time information & the range of indecis.
	while(i<table->GetNumberOfRows() && tableDate <= unixts){
		if(tableDate == unixts){
			if(starttime < 0){
				starttime = i; //set starting range
				endtime = i; // set ending range
			}else{
				endtime = i; // increment ending range...
			}
		}else{
			prevTime = tableDate;
		}

			i++;
			tableDate = table->GetValueByName(i,"UnixTimestamp").ToInt();
	}

	//If the given was for initialization or the given date/time < date/time in the Table record
	if(prevTime < 0){
		prevTime = table->GetValueByName(0,"UnixTimestamp").ToInt(); // Set the time to be the first date/time on the table 
	}

	if(starttime < 0 && endtime < 0){ // NO Valid indices range
		getTableIndexUTS(table, prevTime, index); // find the row index range of the table with previous/new date/time
	}else{ // Found Valid index range

		//insert indices into the vector
		index->push_back(starttime); 
		index->push_back(endtime);
	}
}// getTableIndexUTS()

/*
 * Get data from the network Table; Create necessary vertices & edges and draw a graph.
 * Input: starting index of network Table, ending index of network Table to get data from the network Table
 * Return 1
 */
int drawGraph(int beginAt, int endAt){

	/*************************************************/
	/*** Remove Previous Graph Data & Reset Arrays ***/
	/*************************************************/

	g->RemoveVertices(vertexArray); // remove all the vertices except one for the AFC network systems.

	std::cout<<std::endl;
	vertexArray->Reset();

	vertexName->Reset();
	vertexColour->Reset();
	edgeWeight->Reset();
	edgeColour->Reset();

	// Restore neccesary information
	for(int r = 0; r < vertexT->GetNumberOfRows(); r++){
		//If not a part of AFC Network System, reset the pedigreID to -1 (since no IPAddress has vertecies.)
		if(vertexT->GetValueByName(r, "Type").ToString() == "IP Address"){
			if(vertexT->GetValueByName(r, "PedigreeID").ToInt() > -1){
				vertexT->SetValueByName(r,"PedigreeID",-1);
			}
		}else{
			//Restore vertecis  & edges data of AFC Network System's verticies/edges 
			vertexName->InsertNextValue(vertexT->GetValueByName(r,"Name").ToString());
			vertexColour->InsertNextValue(vertexT->GetValueByName(r,"VertexColour").ToInt());
			edgeWeight->InsertNextValue("AFC Network");
			edgeColour->InsertNextValue(1);
		}
	}

	/***********************************************************/
	/*** Remove Vertices from the previous date/time display ***/
	/***********************************************************/

	// from the beginAt index to endAT index
	// get data from the network Table.
	for(int i = beginAt; i < endAt+1 ; i++){
		//get address/port#y.
		std::string src = networkT->GetValueByName(i, "srcAddr").ToString(); // get source IP Address
		std::string dst = networkT->GetValueByName(i, "dstAddr").ToString(); // get destination IP Address

		//Check If the vertex information exists in the vertex Table, 
		int srcV = vertexExist(src); // source IP's vertex-table index
		int dstV = vertexExist(dst); // destination IP's vertex-table index
		int srcpid = -1, dstpid = -1;

		/* if Not vertex on the graph
		 *		create vertex and store the pedigreeID information in the vertexID
		 *		Also, add vertex's name & colour information in the Array.
		 * else get the pedigreeID (AFC Network Servers)
		 */

		 //for the Source IP Address
		if(vertexT->GetValueByName(srcV,"PedigreeID").ToInt() == -1){
			srcpid = g->AddVertex();
			vertexT->SetValueByName(srcV, "PedigreeID", srcpid);
			vertexArray->InsertNextValue(srcpid);

			vertexName->InsertNextValue(vertexT->GetValueByName(srcV,"Name").ToString());
			vertexColour->InsertNextValue(vertexT->GetValueByName(srcV,"VertexColour").ToInt());
		}else{
			srcpid = get_pid(src);
		}

		//for the Destination IP Address
		if(vertexT->GetValueByName(dstV,"PedigreeID").ToInt() == -1){
			dstpid = g->AddVertex();
			vertexT->SetValueByName(dstV, "PedigreeID", dstpid);
			vertexArray->InsertNextValue(dstpid);

			vertexName->InsertNextValue(vertexT->GetValueByName(dstV,"Name").ToString());
			vertexColour->InsertNextValue(vertexT->GetValueByName(dstV,"VertexColour").ToInt());
		}else{
			dstpid = get_pid(dst);
		}

		//create a link between the sourceIP & destinationIP
		g->AddEdge(srcpid, dstpid);

		// Depend on the Network operation, add different edgeColour information
		//For Display on the window with the graph. 
		if(networkT->GetValueByName(i, "Operation").ToString() == "Deny"){
			edgeColour->InsertNextValue(3);
		}else{
			edgeColour->InsertNextValue(2);
		}
		edgeWeight->InsertNextValue(networkT->GetValueByName(i, "Weight").ToString());
	}
	
	// print out the corresponding data of the graph on the console..
	std::cout<<"Firewall Log Information:"<<std::endl;
	for(int i = beginAt; i< endAt+1; i++){
		vtkSmartPointer<vtkVariantArray> row = networkT->GetRow(i);

		for(int j =0; j<row->GetNumberOfValues(); j++){
			std::cout << row->GetValue(j) << " || ";
		}
		std::cout<<std::endl;
	}
	std::cout<<std::endl;
	/*for(int i = 0; i< (int)vertexT->GetNumberOfRows(); i++){
		vtkSmartPointer<vtkVariantArray> rrow = vertexT->GetRow(i);
		if(rrow->GetValue(0) >=0){
			for(int j =0; j<rrow->GetNumberOfValues(); j++){
				std::cout << rrow->GetValue(j) << " || ";
			}
			std::cout<<std::endl;
		}
	}*/

	return 1;
}// drawGraph()

/*
 * For the Input Argument logfile, open and read line by line
 *		Split information & Store necessary data into the networkTable
 * Input: Logfile to open, to read & to store data into the table
 * Return 0 if it failed.
 */
int FirewallLogData(char* logfile){

	std::ifstream f(logfile);
	std::string line;

	//read line each lines in the firewall-Log file;
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


			if(spacefound != std::string::npos){ // IF not a first data description row;
				std::string datetime = token[0];
				std::stringstream intostr;
				
				// Convert file-text date/time string in unix_timestamp time.
				int unixts = (int)toUnixTimestamp(datetime);

				intostr << unixts;

				// instead of the file-tex date/time string,
				// insert unix_timestamp time.
				token.erase(token.begin());
				token.insert(token.begin(),intostr.str());

				//token = [UnixTimestamp][op][srcAddr][dstAddr][srcPort][dstPort]
				// If source IP Address exists,
				if(token[2] != "(empty)"){
					if((int)networkT->GetNumberOfRows() == 0){
						addNetworkT(token);
					
					}else{
						// check for duplication
						int row = networkExist(token);

						if(row < 0){ // No duplication, create a new row data in the network Table.
							addNetworkT(token);
						
						}else{ // Duplicated information; increase the weight of the edge/link.
							int atRow = row;
							//int weightCol = 7;
							
							int w = networkT->GetValueByName(atRow, "Weight").ToInt();
							networkT->SetValueByName(atRow, "Weight", w+1); // increase the weight by one
						}
					}
				}
			}

		}catch(std::exception e){
			std::cout << "Try-Catch Exception called at FirewallLogData()" << std::endl;
			return 0;
		}
	}
	return 1;
}//FirewallLogData()

/*
 * Check the Initialization or Keyboard input
 * Draw a graph with the network Table data
 * Input: String type of Keyboard Input
 */
int GraphNetworkFlow(std::string datetime){
	std::size_t none = (size_t)(-1);
	std::string dateInfo,timeInfo;
	std::size_t space = datetime.find_first_of(" ",0);

	// Check STDIN Keyboard Input
	// If the keyboard input is in the correct format, continue;
	// Else return 0.
	if(space != none){ // date & time
		dateInfo = datetime.substr(0,space);
		timeInfo = datetime.substr(space+1, datetime.length());

		//Input Format Error
		if(dateInfo.find("/",0) == none || timeInfo.find(":",0) == none){
			std::cout<< "stdin Format Error. \nUsage: <dd/Mmm(in Alphabetic)/yyyy [optional: hh/mm/ss]>";
			std::cout<<"\ne.g. 13/Apr/2011 08:52:53 or 13/Apr/2011" << std::endl;
			return 0;
		}

	}else if(datetime.find("/",0) != none && datetime.find(":",0) == none){ // only date
		datetime += " 00:00:00";
	}else{ // no date = Input Format Error
		std::cout<< "stdin Format Error. \nUsage: <dd/Mmm(in Alphabetic)/yyyy [optional: hh/mm/ss]>";
		std::cout<<"\ne.g. 13/Apr/2011 08:52:53 or 13/Apr/2011" << std::endl;
		return 0;
	}

	/*****************************************/
	/*** Correct Keyboard input 'datetime' ***/
	/*****************************************/
	
	std::string utimestamp = "";
	// convert keyboard/initialization input into unixtimestamp format.
	if(datetime == "dd/mmm/yyyy hh:mm:ss"){ //initial
		utimestamp = networkT->GetValueByName(0,"UnixTimestamp").ToString();
	}else{
		int tempdatetime = (int)toUnixTimestamp(datetime);
		utimestamp= toString(tempdatetime);
	}

	displayDATE = utimestamp; // Set current display Date/time

	// Get the range of the indices from the network Table to create edges of the graph
	std::vector<int> index;
	getTableIndexUTS(networkT, toInt(utimestamp),&index);
	int beginAt=index[0], endAt=index[1];
	
	//If the range of the indices are valid, create an undirect-graph with data from the network Table.
	if(beginAt >=0 && endAt >= 0){
		std::cout<<datetime << " (" << beginAt << ", " << endAt << ")" << std::endl;
		drawGraph(beginAt, endAt);
	}else{
		std::cout<< datetime << "No data available at (GraphNetworkFlow())" << std::endl;
		return 0;
	}
	return 1;
}//GraphNetworkFlow

/*
 * Display two windows (one for xy-plot graph & another one for the undirect-graph)
 * Undirect-graph will have an slider to display different graph depend on the time.
 */
int Display(int xyplot){

	/*****************************************/
  	/********* Chart XY Plot Graph ***********/
  	/*****************************************/
	if(xyplot == 1){
		vtkSmartPointer<vtkContextView> Cview = vtkSmartPointer<vtkContextView>::New();
	  	Cview->GetRenderer()->SetBackground(1.0, 1.0, 1.0);

	  	vtkSmartPointer<vtkChartXY> ChartXY = vtkSmartPointer<vtkChartXY>::New();
	  	Cview->GetScene()->AddItem(ChartXY);
	  	ChartXY->SetShowLegend(true);
	  	vtkPlot* Plot;
	  	
	  	//For Each Column in the plot Table, draw a seperate line graph

	  	//FS1
	  	Plot = ChartXY->AddPlot(vtkChart::LINE);
	  	Plot->SetInputData(plotT,0,1);
	  	//Plot->SetColor(255,0,0,0);
	  	Plot->SetWidth(2.0);

	  	//EX1
	  	Plot = ChartXY->AddPlot(vtkChart::LINE);
	  	Plot->SetInputData(plotT,0,2);
	  	//Plot->SetColor(1,0,1,0);
	  	Plot->SetWidth(2.0);

	  	//WEB
	  	Plot = ChartXY->AddPlot(vtkChart::LINE);
	  	Plot->SetInputData(plotT,0,3);
	  	Plot->SetColor(0.0,0.0,1.0,0);
	  	Plot->SetWidth(2.0);

	  	Plot = ChartXY->AddPlot(vtkChart::LINE);
	  	Plot->SetInputData(plotT,0,4);
	  	//Plot->SetColor(0,255,255,0);
		Plot->SetWidth(2.0);

	  	Plot = ChartXY->AddPlot(vtkChart::LINE);
	  	Plot->SetInputData(plotT,0,5);
	  	//Plot->SetColor(0,255,0,0);
	  	Plot->SetWidth(2.0);

	  	Plot = ChartXY->AddPlot(vtkChart::LINE);
	  	Plot->SetInputData(plotT,0,6);
	  	//Plot->SetColor(255,255,0,0);
	  	Plot->SetWidth(2.0);

	  	Plot = ChartXY->AddPlot(vtkChart::LINE);
	  	Plot->SetInputData(plotT,0,7);
	  	//Plot->SetColor(0,0,0,0);
	  	Plot->SetWidth(2.0);

	  	Plot->SetUseIndexForXSeries(false);
	  	Cview->GetInteractor()->Initialize();
	  	Cview->GetInteractor()->Start();
	}

  	/*****************************************/
  	/********** Un-Direct G R A P H **********/
  	/*****************************************/

	vtkSmartPointer<vtkGraphLayoutView> GLview = vtkSmartPointer<vtkGraphLayoutView>::New();

	vtkSmartPointer<vtkGraphLayout> GraphLayout = vtkSmartPointer<vtkGraphLayout>::New();
	vtkSmartPointer<vtkSimple2DLayoutStrategy> strategy = vtkSmartPointer<vtkSimple2DLayoutStrategy>::New();
	GraphLayout->SetInputData(g);
	GraphLayout->SetLayoutStrategy(strategy);

	GLview->SetLayoutStrategyToSimple2D(); // SetLayoutStrategyToPassThrough();
	GLview->SetEdgeLayoutStrategyToPassThrough();
	GLview->AddRepresentationFromInputConnection(GraphLayout->GetOutputPort());

	//vtkSmartPointer<vtkGraphToPolyData> GraphToPolyData = vtkSmartPointer<vtkGraphToPolyData>::New();
	//GraphToPolyData->SetInputConnection(GraphLayout->GetOutputPort());
	//GraphToPolyData->EdgeGlyphOutputOn();
	//GraphToPolyData->SetEdgeGlyphPosition(1);

	//vtkSmartPointer<vtkGlyphSource2D> GlyphSource2D = vtkSmartPointer<vtkGlyphSource2D>::New();
	//GlyphSource2D->Update();

	//vtkSmartPointer<vtkGlyph3D> Glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
	//Glyph3D->SetInputConnection(0, GraphToPolyData->GetOutputPort(1));
	//Glyph3D->SetInputConnection(1, GlyphSource2D->GetOutputPort());

	//vtkSmartPointer<vtkPolyDataMapper> PolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	//PolyDataMapper->SetInputConnection(Glyph3D->GetOutputPort());
	
	//vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	//actor->SetMapper(PolyDataMapper);

	//-----
	//GLview->AddRepresentationFromInput(g);
	GLview->VertexLabelVisibilityOn();
	GLview->SetVertexLabelArrayName("vertexName");
	/*GLview->ColorVerticesOn();
	GLview->SetVertexColorArrayName("vertexColour");*/
	
	GLview->EdgeLabelVisibilityOn();
	GLview->SetEdgeLabelArrayName("edgeWeight");
	//GLview->ColorEdgesOn();
	//GLview->SetEdgeColorArrayName("edgeColour");

	vtkSmartPointer<vtkViewTheme> ViewTheme = vtkSmartPointer<vtkViewTheme>::New();
	ViewTheme->SetPointLookupTable(lookupT);
	GLview->ApplyViewTheme(ViewTheme);

	//GLview->GetRenderer();//->AddActor(actor);

	//GLview->ResetCamera();
	//GLview->GetInteractor()->Initialize();
	//GLview->Render();

	/**************************************/
	vtkSmartPointer<vtkSliderRepresentation3D> slider = vtkSmartPointer<vtkSliderRepresentation3D>::New();
	slider->SetMinimumValue(0);
	slider->SetMaximumValue(3.0);
	slider->SetValue(0);
	slider->SetTitleText("Date/Time");
	slider->SetTitleHeight(0.03);
	slider->SetLabelHeight(0.01);
	slider->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
	slider->GetPoint1Coordinate()->SetValue(-3,-5,0);
	slider->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
	slider->GetPoint2Coordinate()->SetValue(3,-5,0);

	slider->SetSliderLength(0.5);
	slider->SetSliderWidth(0.03);
 	slider->SetEndCapLength(0.03);

 	vtkSmartPointer<vtkSliderWidget> sliderWidget = vtkSmartPointer<vtkSliderWidget>::New();
	sliderWidget->SetInteractor(GLview->GetInteractor());
	sliderWidget->SetRepresentation(slider);
	sliderWidget->SetAnimationModeToAnimate();
	sliderWidget->EnabledOn();
	
	vtkSmartPointer<vtkSliderCallback> callback = vtkSmartPointer<vtkSliderCallback>::New();
	callback->MutableUndirectedGraph = g;
	sliderWidget->AddObserver(vtkCommand::InteractionEvent,callback);
	
	GLview->Update();
	GLview->ResetCamera();
	GLview->GetInteractor()->Initialize();
	GLview->Render();
	/**************************************/

	GLview->GetInteractor()->Start();
}// Display()


/*
 * main()
 * Arguments: firewall_log(s) , IDS_log(s) ,  
 */
int main(int argc, char* argv[]){

	if(argc < 3){
		std::cout << "Usage: ./networkop_afc <firewall_log1> <firewall_log2> ...... <firewall_logn> <,>"<< std::endl;
		return 0;
	}

	// Set global Table variables
	SetUpTable();

	//set All Freight Corporation Netowrk Nodes
	AFCnetwork();
	std::cout<<"Created All Freight Corporation Network Nodes." << std::endl;

	// To discriminate firewall_log(s) input files; get the index of the commas
	int comma1 = -1;
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i],",") == 0){
			if (comma1 == -1){
				comma1 = i;
			}
		}
	}

	//Read Firewall_log and store necessary information into the Vertex & Network Tables
	std::cout << "Processing FirewallLog Data" << std::endl;
	for(int fw = 1; fw < comma1; fw++){
		char* firewall_log = argv[fw];
		FirewallLogData(firewall_log);
	}

	// Store information for the xyplot Plot Table
	plotTable();
	
	// Initialization on the Undirect Graph
	// Create necessary nodes & links for graph
	GraphNetworkFlow("dd/mmm/yyyy hh:mm:ss"); //input format: dd/Apr/2011 hh:mm:ss (24hr)

	// Set graph's vertices & edges extra information.
	g->GetVertexData()->AddArray(vertexName);
	//g->GetVertexData()->AddArray(vertexColour);
	g->GetEdgeData()->AddArray(edgeWeight);
	//g->GetEdgeData()->AddArray(edgeColour);	

	std::cout << "Processing to Dispaly Graph" << std::endl;
	Display(1); // Display Graph(s)

	//Display User's choice of the Graph...
	std::string keyboard;
	getline (std::cin,keyboard);
	while(keyboard != "q" || keyboard != "exit"){
		
		std::string inputDate,inputTime;
		std::size_t spacefound = keyboard.find_first_of(" ",0);
	
		// split date & time
		if(spacefound != std::string::npos){ // Keybaord input: date & time
			inputDate = keyboard.substr(0,spacefound);
			inputTime = keyboard.substr(spacefound+1, keyboard.length());
			if(inputDate.find("/",0) != std::string::npos || inputTime.find(":",0) != std::string::npos){
				GraphNetworkFlow(keyboard);
				Display(0);

			}else{
				std::cout<< "stdin Format Error. \nUsage: <dd/Mmm(in Alphabetic)/yyyy [optional: hh/mm/ss]>";
				std::cout<<"\ne.g. 13/Apr/2011 08:52:53 or 13/Apr/2011" << std::endl;
			}
		}else if(keyboard.find("/",0) != std::string::npos && keyboard.find(":",0) == std::string::npos){ // Keyboard Input: only date
			inputDate = keyboard + " 00:00:00";
			GraphNetworkFlow(inputDate);
			Display(0);
		}else{ // Keyboard Input: no date
			std::cout<< "stdin Format Error. \nUsage: <dd/Mmm(in Alphabetic)/yyyy [optional: hh/mm/ss]>";
			std::cout<<"\ne.g. 13/Apr/2011 08:52:53 or 13/Apr/2011" << std::endl;
		}
		getline (std::cin,keyboard);
	}

	/*for(int i = 0; i< (int)vertexT->GetNumberOfRows(); i++){
		vtkSmartPointer<vtkVariantArray> rrow = vertexT->GetRow(i);

		for(int j =0; j<rrow->GetNumberOfValues(); j++){
			std::cout << rrow->GetValue(j) << " || ";
		}
		std::cout<<std::endl;
	}

	for(int i = 0; i< (int)networkT->GetNumberOfRows(); i++){
		vtkSmartPointer<vtkVariantArray> row = networkT->GetRow(i);

		for(int j =0; j<row->GetNumberOfValues(); j++){
			std::cout << row->GetValue(j) << " || ";
		}
		std::cout<<std::endl;
	}*/

  	return 0;
} // main()
