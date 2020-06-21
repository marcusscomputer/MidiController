
// Controller daemon for the Akai Midi Mix device

#include <string>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iterator>
#include <fstream>
#include <limits>
#include <unistd.h>


// A vector containing the channel numbers for the sliders
std::vector<int> sliderChannels = {127, 126, 125, 124, 63, 122, 121, 120};

// A vector containing the knob channels
std::vector<int> knobChannels = {
    99, 20, 24, 60, 93, 92, 106, 105,
    17, 21, 25, 61, 94, 91, 104, 103,
    80, 81, 82, 2, 95, 90, 102, 101
};


// Master slider channel
int masterSlider = 62;


// Which bank we are currently at:
int bank  = 0;
// Size of each bank:
int bsize = 8;


// Making sure numbers are not insanely increased
std::string prv_line = "";

// Last sent command
std::string last_command = "";

// Making things easy
std::string bankShiftLeft  = "80 1A 7F";
std::string bankShiftRight = "80 19 7F";


// Needed to split strings
template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}



std::istream& ignoreline(std::ifstream& in, std::ifstream::pos_type& pos)
{
    pos = in.tellg();
    return in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string getLastLine(std::ifstream& in)
{
    std::ifstream::pos_type pos = in.tellg();

    std::ifstream::pos_type lastPos;
    while (in >> std::ws && ignoreline(in, lastPos))
        pos = lastPos;

    in.clear();
    in.seekg(pos);

    std::string line;
    std::getline(in, line);
    
    std::string read = "";
    if (line != prv_line) { read = line; }
    
    return read;
}


int hexToInt(std::string hex)
{
    char * p;
    int n = strtol( hex.c_str(), & p, 16 );
    return n;
}


void sendSliderOSC(int track, int value, bool master, bool isKnob, int knob)
{
    float v = (float)value / 127.0f;
    std::string cmd = "sendosc 127.0.0.1 9000 ";
    
    if (master == true)
    { cmd.append("/master"); }
    if (master == false && isKnob == false)
    {
        cmd.append("/track/");
        cmd.append(std::to_string((bsize*bank)+track));
    }
    if (master == false && isKnob == true)
    { cmd.append("/knob/"); cmd.append(std::to_string(knob)); }
    
    cmd.append("/volume f ");
    cmd.append(std::to_string(v));
    if (cmd != last_command)
    { system(cmd.c_str()); last_command = cmd; }
    //{ std::cout << cmd << "\n"; last_command = cmd; }
}


// Increase or decrease current bank of sliders
void shiftBankLeft()  { bank--; if (bank == -1) {bank = 0;} }
void shiftBankRight() { bank++; }


// This checks for any input done on device
void monitorMidiInput()
{
    std::ifstream file("/tmp/akaiMidiMixData");
    std::string lastLine = getLastLine(file);
    
    if (prv_line != bankShiftLeft && lastLine != bankShiftRight && lastLine != "")
    {
        // First check if the banks have been shifted instead
        if (lastLine == "90 1A 7F") { shiftBankRight(); return; }
        if (lastLine == "90 19 7F") { shiftBankLeft();  return; }

        // lastLine is MIDI data. Split it first:
        std::vector<std::string> midiData = split(lastLine, ' ');

        // Go through content, and present us with the channel and the value
        int chn = hexToInt(midiData[1]);

        // Figure out which track that is:
        int trk = -1;
        for (int i = 0; i<sliderChannels.size(); i++)
        { if (sliderChannels[i] == chn) { trk = i+1; } }
        
        // Switch for a knob
        bool isKnob = false; int knob = -1;
        for (int i=0; i<knobChannels.size(); i++)
        { if (knobChannels[i] == chn) { isKnob = true; knob = chn;} }
        
        // Switch for master channel
        bool isMaster = false;
        if (chn == 62) { isMaster = true; }

        // And value to be used
        int val = hexToInt(midiData[2]);

        // OK! Send to OSC:
        sendSliderOSC(trk, val, isMaster, isKnob, knob);
    }

    // Note this line
    prv_line = lastLine;
}




int main (int argc, char** argv[])
{
    bool run = true;
    long timer = 0;
    
    while (run == true)
    {
        monitorMidiInput();
        usleep(50000);
    }

    return 0;
}


