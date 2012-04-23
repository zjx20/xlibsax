#include <iostream> 
#include <sax/xml/ticpp.h> 
 
// simple function to parse a xml file 
void Parse(const char* pcFilename); 
 
int main() 
{ 
        // whatch out for exceptions 
        try 
        { 
                // parse the xml file with the name fruits.xml 
                Parse("fruits.xml"); 
        } 
        catch(ticpp::Exception& ex) 
        { 
                std::cout << ex.what(); 
        } 
 
        return -1; 
} 
 
void Parse(const char* pcFilename) 
{ 
        // first load the xml file 
        ticpp::Document doc(pcFilename); 
        doc.LoadFile(); 
 
        // parse through all fruit elements 
        ticpp::Iterator<ticpp::Element> child; 
        for(child = child.begin(doc.FirstChildElement()); child != child.end(); child++) 
        { 
                // The value of this child identifies the name of this element 
                std::string strName; 
                std::string strValue; 
                child->GetValue(&strName); 
                std::cout << "fruit: " << strName << std::endl; 
                std::cout << "-------------" << std::endl; 
 
                // now parse through all the attributes of this fruit 
                ticpp::Iterator< ticpp::Attribute > attribute; 
                for(attribute = attribute.begin(child.Get()); attribute != attribute.end(); 
attribute++) 
                { 
                        attribute->GetName(&strName); 
                        attribute->GetValue(&strValue); 
                        std::cout << strName << ": " << strValue << std::endl; 
                } 
                std::cout << std::endl; 
        } 
}
