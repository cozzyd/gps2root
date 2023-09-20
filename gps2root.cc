#define __GNU_SOURCE
#include <cstdio>
#include <cstdlib> 
#include <iostream> 
#include <cfloat>
#include <unistd.h> 
#include "TFile.h" 
#include "TTree.h" 
#include <vector> 
#include <nlohmann/json.hpp>


void usage()
{

  printf("usage: gps2root [-hfv] infile [infile2] [infile...] outfile.root \n"); 

}

const int leaps[] = { 
  46828800, 78364801, 109900802, 173059203, 252028804, 315187205,
  346723206, 393984007, 425520008, 457056009, 504489610, 551750411, 599184012,
  820108813, 914803214, 1025136015, 1119744016, 1167264017
}; 

const int nleaps = sizeof(leaps)/sizeof(*leaps); 

int count_leaps(int gps_time) 
{
  int leapsecs=18; 
  for(int ileap = nleaps-1; ileap >=0; ileap--) 
  {
    if (gps_time < leaps[ileap])
    {
      leapsecs--; 
    }
    else break; 
  }
  return leapsecs; 
}

struct gpstime
{
  int ns = 0;
  time_t unixtime=0; 
  double itow=0; 
};

template<typename T> 
gpstime extract_time(T js, const std::string key = "time") 
{
  auto val = js[key]; 
  gpstime ret; 
  if (val.is_null())
  {
    return ret; 
  }
  std::string t = val.template get<std::string>(); 
  int year,mon,day,hour,min; 
  double sec; 
  
  sscanf(t.c_str(),"%04d-%02d-%02dT%02d:%02d:%lfZ",&year,&mon,&day,&hour,&min,&sec); 

  ret.ns = 1e9*(sec - int(sec)); 


  struct tm tmtim;
  tmtim.tm_year = year-1900; 
  tmtim.tm_mon = mon-1; 
  tmtim.tm_mday = day; 
  tmtim.tm_hour = hour; 
  tmtim.tm_min = min; 
  tmtim.tm_sec = int(sec); 
  tmtim.tm_isdst= 0; 
  tmtim.tm_gmtoff = 0; 

  ret.unixtime = mktime(&tmtim); 
  ret.itow = tmtim.tm_wday * 24 * 3600 + hour * 3600 + min * 60 + sec; 
  return ret; 

}





//x macro for things copied directly from each satellite (ignoring header) 


  
#define RAW_MSG_EACH\
  X(int, gnssid,-1)      \
  X(int, svid,-1)        \
  X(int, snr,-1)         \
  X(std::string, obs,"") \
  X(int, lli,-1) \
  X(int, locktime,-1) \
  X(double, pseudorange,-1) \
  X(double, carrierphase,-DBL_MAX) \
  X(double, doppler,-DBL_MAX) 


#define RAW_MSG_HDR\
 X(int,gps_time,-1) \
 X(time_t,unix_time,-1) \
 X(int,itow,-1) \
 X(int,nsecs,-1)

#define RAW_MSG \
  RAW_MSG_HDR \
  RAW_MSG_EACH

#define X(type, what,dflt)   type what = dflt;
struct raw_msg
{
  RAW_MSG
};

raw_msg raw; 

#define SKY_MSG_DIRECT_COPY\
  X(float,gdop,-1)\
  X(float,hdop,-1)\
  X(float,pdop,-1)\
  X(float,tdop,-1)\
  X(float,xdop,-1)\
  X(float,ydop,-1)\
  X(float,vdop,-1)\
  X(int,nSat,-1)\
  X(int,uSat,-1)

#define SKY_MSG_EACH\
  X(int,PRN,-1)\
  X(int,gnssid,-1)\
  X(int,svid,-1)\
  X(float,az,-999)\
  X(float,el,-999)\
  X(float,prRes,-FLT_MAX)\
  X(float,ss,-1)\
  X(int,used,-1)\
  X(int,health,-1)

#define SKY_MSG_HDR\
  X(double,itow,-1)

#define SKY_MSG\
  SKY_MSG_HDR\
  SKY_MSG_DIRECT_COPY\
  SKY_MSG_EACH

struct sky_msg
{
  SKY_MSG
};

sky_msg sky; 

#define TPV_MSG_HDR\
 X(double, unix_time,-1)\
 X(double, itow,-1)

#define TPV_MSG_DIRECT_COPY\
 X(int,mode,-1)\
 X(int,leapseconds,-1)\
 X(double,ept,-DBL_MAX)\
 X(double,epx,-DBL_MAX)\
 X(double,epy,-DBL_MAX)\
 X(double,epv,-DBL_MAX)\
 X(double,lat,-DBL_MAX)\
 X(double,lon,-DBL_MAX)\
 X(double,altHAE,-DBL_MAX)\
 X(double,altMSL,-DBL_MAX)\
 X(double,alt,-DBL_MAX)\
 X(double,track,-DBL_MAX)\
 X(double,magtrack,-DBL_MAX)\
 X(double,magvar,-DBL_MAX)\
 X(double,speed,-DBL_MAX)\
 X(double,climb,-DBL_MAX)\
 X(double,eps,-DBL_MAX)\
 X(double,epc,-DBL_MAX)\
 X(double,ecefx,-DBL_MAX)\
 X(double,ecefy,-DBL_MAX)\
 X(double,ecefz,-DBL_MAX)\
 X(double,ecefvx,-DBL_MAX)\
 X(double,ecefvy,-DBL_MAX)\
 X(double,ecefvz,-DBL_MAX)\
 X(double,ecefpAcc,-DBL_MAX)\
 X(double,ecefvAcc,-DBL_MAX)\
 X(double,geoidSep,-DBL_MAX)\
 X(double,eph,-DBL_MAX)\
 X(double,sep,-DBL_MAX)

#define TPV_MSG\
  TPV_MSG_HDR\
  TPV_MSG_DIRECT_COPY


struct tpv_msg
{
  TPV_MSG
};

tpv_msg tpv; 

#undef X

#define X(type,what,dflt)  t->Branch(#what, &(raw.what));
void setup_raw(TTree *t) 
{
  RAW_MSG
}
#undef X
#define X(type,what,dflt)  t->Branch(#what, &(tpv.what));
void setup_tpv(TTree *t) 
{
  TPV_MSG
}
#undef X
#define X(type,what,dflt)  t->Branch(#what, &(sky.what));
void setup_sky(TTree *t) 
{
  SKY_MSG
}
#undef X



int main(int nargs, char ** args) 
{

   char * outfile = 0; 
   bool verbose = false; 
   std::vector<char*> infiles; 

   bool force = false;
   char * last_file = 0; 

   for (int i = 1; i < nargs; i++) 
   {
      if (!strcmp(args[i],"-h"))
      {
        usage(); 
        return  0; 
      }
      if (!strcmp(args[i],"-f"))
      {
        force = true; 
      }
      if (!strcmp(args[i],"-v"))
      {
        verbose = true; 
      }


      else 
      {
        if (last_file) 
        {
          if (access(last_file, R_OK))
          {
            fprintf(stderr,"can't find or read %s\n", last_file); 
          }
          else
          {
            printf("Using input file %s\n", last_file); 
            infiles.push_back(last_file); 
          }
        }
        last_file = args[i]; 
      }
   }
   if (!infiles.size())
   {
     usage(); 
     return 1; 
   }
   outfile = last_file; 
   printf("Using output file %s\n", outfile); 

   printf("Checking for gpsdecode...\n  "); 
   fflush(stdout); 
   int ret = system("which gpsdecode"); 
   if (ret) 
   {
     fprintf(stderr,"..Not found!\n gpsdecode is required.") ; 
     return 1; 
   }
 
   TFile fout(outfile, force ? "RECREATE" : "CREATE"); 
   TTree  * raw_tree = 0; 
   TTree  * tpv_tree = 0; 
   TTree  * sky_tree = 0; 

   for (auto infile : infiles) 
   {
     std::string cmd = "gpsdecode < ";
     cmd+= infile; 
     std:: cout << "Running: " << cmd << std::endl; 
     FILE * pipe = popen(cmd.c_str(),"r"); 

     char * line = 0; 
     size_t N = 0; 

     while (getline(&line, &N, pipe) > -1)
     {
       auto fragment = nlohmann::json::parse(line); 

       std::string cls = fragment["class"].template get<std::string>(); 
       if (verbose) std:: cout << "Found: " << cls << std::endl; 
       if (verbose) std::cout << fragment << std::endl;; 

       if (cls == "SKY")
       {
         if (!sky_tree) 
         {
           sky_tree = new TTree("sky","sky"); 
           setup_sky(sky_tree); 
         }

         gpstime t = extract_time(fragment); 
         sky.itow = t.itow; 

#define X(type,what,dflt)  auto what##val = fragment[#what]; sky.what = what##val.is_null() ? dflt : what##val.template get<type>(); 
         SKY_MSG_DIRECT_COPY
#undef X
#define X(type,what,dflt) auto what##val = fragment["satellites"][i][#what];  sky.what = what##val.is_null() ? dflt : what##val.template get<type>(); 
         int N = fragment["satellites"].size(); 
         for (int i = 0; i < N; i++) 
         {
           if (verbose) std::cout << "sat" << i <<":  "<< fragment["satellites"][i] << std::endl;; 
           SKY_MSG_EACH
           sky_tree->Fill(); 
         }
#undef X
       }

       else if (cls == "TPV")
       {
         if (!tpv_tree) 
         {
           tpv_tree = new TTree("tpv","tpv"); 
           setup_tpv(tpv_tree); 
         }

         gpstime t = extract_time(fragment); 
         tpv.unix_time = t.unixtime + t.ns * 1e-9;  
         tpv.itow = t.itow; 

#define X(type,what,dflt)  auto what##val = fragment[#what]; tpv.what = what##val.is_null() ? dflt : what##val.template get<type>(); 
         TPV_MSG_DIRECT_COPY
#undef X
         tpv_tree->Fill(); 
       }

       else if (cls == "RAW")
       {
         if (!raw_tree) 
         {
           raw_tree = new TTree("raw","raw"); 
           setup_raw(raw_tree); 
         }

         raw.gps_time = fragment["time"].template get<int>(); 
         raw.unix_time = raw.gps_time-count_leaps(raw.gps_time); 
         raw.nsecs = fragment["nsec"].template get<int>(); 
         struct tm *tmtim  = gmtime(&raw.unix_time); 
         raw.itow = 3600*24 * tmtim->tm_wday + 3600 * tmtim->tm_hour + 60 * tmtim->tm_min + tmtim->tm_sec + raw.nsecs * 1e-9; 
#define X(type,what,dflt)  auto what##val = fragment["rawdata"][i][#what]; raw.what = what##val.is_null() ? dflt : what##val.template get<type>(); 
         int N = fragment["rawdata"].size(); 
         for (int i = 0; i < N; i++) 
         {
           if (verbose) std::cout << "sat" << i << ": " << fragment["rawdata"][i] << std::endl;; 
           RAW_MSG_EACH
           raw_tree->Fill(); 
         }
#undef X
       }
     }
   }
   fout.Write(); 
}




