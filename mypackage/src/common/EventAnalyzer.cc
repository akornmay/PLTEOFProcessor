#include "bril/mypackage/EventAnalyzer.h"
using namespace std;

EventAnalyzer::EventAnalyzer(PLTEvent *evt, std::string alignmentFile, std::string trackFile, vector<unsigned> channels)
{
  // Initialize the beam crossing counter
  _bxCounter = 0;

  // Point to the event object
  _event = evt; 

  // Set the fiducial detector region
  //_fidRegionHits = PLTPlane::kFiducialRegion_FullSensor;
  _fidRegionHits = PLTPlane::kFiducialRegion_All;
  _event->SetPlaneFiducialRegion(_fidRegionHits);
  _event->SetPlaneClustering(PLTPlane::kClustering_AllTouching, PLTPlane::kFiducialRegion_All);
  _event->SetTrackingAlgorithm(PLTTracking::kTrackingAlgorithm_01to2_AllCombs);

  // Read in alignment file
  _alignment = new PLTAlignment();
  // if (alignmentFile == "") {
  //     _alignment->ReadAlignmentFile("/nfshome0/naodell/plt/daq/bril/pltslinkprocessor/data/Trans_Alignment_2017MagnetOn_Prelim.dat");
  // } else {
  //     _alignment->ReadAlignmentFile(alignmentFile);
  // }
  std::cout << alignmentFile << std::endl;
  _alignment->ReadAlignmentFile(alignmentFile);

  // Track quality cuts 
  _pixelDist  = 5;//<-Original
  _slopeYLow  = 0.027 - 0.01;
  _slopeXLow  = 0.0 - 0.01;
  _slopeXHigh = 0.0 + 0.01;
  _slopeYHigh = 0.027 + 0.01;

  // initialize counters and get track quality data
  cout << "Reading track quality data... " << endl;
  //ifstream file("/cmsnfshome0/nfshome0/naodell/plt/daq/bril/pltslinkprocessor/data/tracks_2017.csv");
  ifstream file(trackFile.c_str());
  unsigned channel;
  float slopeXMean, slopeXSigma, slopeYMean, slopeYSigma;
  float residualXMean0, residualXSigma0; 
  float residualXMean1, residualXSigma1; 
  float residualXMean2, residualXSigma2; 
  float residualYMean0, residualYSigma0; 
  float residualYMean1, residualYSigma1; 
  float residualYMean2, residualYSigma2; 
  string line;
  unsigned count = 0;
  while (getline(file, line)) {
    if (count == 0) {
      ++count;
      continue;
    } else {
      istringstream iss(line);
      iss >> channel 
	//>> slopeXMean >> slopeXSigma 
	  >> slopeYMean >> slopeYSigma
	  >> slopeXMean >> slopeXSigma 
	  >> residualYMean0 >> residualYSigma0 
	  >> residualYMean1 >> residualYSigma1 
	  >> residualYMean2 >> residualYSigma2
	  >> residualXMean0 >> residualXSigma0 
	  >> residualXMean1 >> residualXSigma1 
	  >> residualXMean2 >> residualXSigma2; 
      //>> residualYMean0 >> residualYSigma0 
      //>> residualYMean1 >> residualYSigma1 
      //>> residualYMean2 >> residualYSigma2; 

      /* cout << "channel: " << channel << "\n"
	 << slopeXMean << ", " << slopeXSigma << "\n"
	 << slopeYMean << ", " << slopeYSigma << "\n"
	 << residualXMean0 << ", " << residualXSigma0 << "\n"
	 << residualXMean1 << ", " << residualXSigma1 << "\n"
	 << residualXMean2 << ", " << residualXSigma2 << "\n"
	 << residualYMean0 << ", " << residualYSigma0 << "\n"
	 << residualYMean1 << ", " << residualYSigma1 << "\n"
	 << residualYMean2 << ", " << residualYSigma2 << "\n"
	 << endl; */

      _meanSlopeX[channel]        = slopeXMean;
      _meanSlopeY[channel]        = slopeYMean;
      _sigmaSlopeX[channel]       = slopeXSigma;
      _sigmaSlopeY[channel]       = slopeYSigma;
      _meanResidualX[channel][0]  = residualXMean0;
      _meanResidualX[channel][1]  = residualXMean1;
      _meanResidualX[channel][2]  = residualXMean2;
      _meanResidualY[channel][0]  = residualYMean0;
      _meanResidualY[channel][1]  = residualYMean1;
      _meanResidualY[channel][2]  = residualYMean2;
      _sigmaResidualX[channel][0] = residualXSigma0;
      _sigmaResidualX[channel][1] = residualXSigma1;
      _sigmaResidualX[channel][2] = residualXSigma2;
      _sigmaResidualY[channel][0] = residualYSigma0;
      _sigmaResidualY[channel][1] = residualYSigma1;
      _sigmaResidualY[channel][2] = residualYSigma2;
    }
  }

  for (unsigned i = 0; i < channels.size(); ++i) {
    channel = channels[i];
    _telescopeHits[channel]        = EffCounter();
    _telescopeHitsDelayed[channel] = EffCounter();
    _accidentalRates[channel]      = make_pair(0, 0);
  }


  cout << "EventAnalyzer initialization done!" << endl;
};

int EventAnalyzer::AnalyzeEvent()
{
  // Increment the crossing counter
  ++_bxCounter;

  // Loop over all telescopes 
  for (size_t it = 0; it != _event->NTelescopes(); ++it) {
    PLTTelescope* telescope = _event->Telescope(it);
    //std::cout << telescope->NTracks() << std::endl;

    // make them clean events
    if (telescope->NHitPlanes() >= 2 && (unsigned)(telescope->NHitPlanes()) == telescope->NClusters()) {

      // Calculate rates for efficiencies 
      this->CalculateTelescopeRates(0, *telescope);
      this->CalculateTelescopeRates(1, *telescope);
      this->CalculateTelescopeRates(2, *telescope);

      if (telescope->NHitPlanes() == 3) {
	this->CalculateAccidentalRates(*telescope);
      }
    }
  }
  return 0;
}

void EventAnalyzer::CalculateTelescopeRates(unsigned iPlane, PLTTelescope &telescope)
{
  PLTPlane *tags[2] = {0x0, 0x0};
  PLTPlane *probe   = 0x0;
  unsigned channel  = telescope.Channel();
  unsigned ix = 0;
  for (size_t ip = 0; ip != telescope.NPlanes(); ++ip) {
    if (ip == iPlane) {
      probe = telescope.Plane(ip);
    } else {
      tags[ix] = telescope.Plane(ip);
      ++ix;
    }
  }

  if (tags[0]->NClusters() > 0 && tags[1]->NClusters() > 0) {

    PLTTrack twoHitTrack;
    twoHitTrack.AddCluster(tags[0]->Cluster(0));
    twoHitTrack.AddCluster(tags[1]->Cluster(0));

    twoHitTrack.MakeTrack(*_alignment);
    // record two-hit track slopes
    std::pair<float, float> slopes;
    slopes = std::make_pair(twoHitTrack.fTVX/twoHitTrack.fTVZ, twoHitTrack.fTVY/twoHitTrack.fTVZ);
    _twoHitTrackSlopes[channel].push_back(slopes);

    // Keep track of number of two/three-hit tracks for this plane
    if (
	// quality cuts
	twoHitTrack.IsFiducial(channel, iPlane, *_alignment, _event->PixelMask()) 
	&& twoHitTrack.NHits() == 2 
                && slopes.first  > _slopeXLow 
                && slopes.first  < _slopeXHigh 
	&& slopes.second > _slopeYLow 
                && slopes.second < _slopeYHigh 
	) {

      std::pair<float, float> LXY;
      std::pair<int, int> PXY;
      PLTAlignment::CP *CP = _alignment->GetCP(channel, iPlane);
      LXY= _alignment->TtoLXY(twoHitTrack.TX(CP->LZ), twoHitTrack.TY(CP->LZ), channel, iPlane);
      PXY= _alignment->PXYfromLXY(LXY);

      ++_telescopeHits[channel].denom[iPlane];
      if (_bxCounter > 1e7) {
	++_telescopeHitsDelayed[channel].denom[iPlane];
      }

      if (probe->NClusters() > 0) {
	std::pair<float, float> ResXY = twoHitTrack.LResiduals(*(probe->Cluster(0)), *_alignment);
	std::pair<float, float> RPXY = _alignment->PXYDistFromLXYDist(ResXY);
	if (fabs(RPXY.first) <= _pixelDist && fabs(RPXY.second) <= _pixelDist) {
	  ++_telescopeHits[channel].numer[iPlane];
	  if (_bxCounter > 1e7) {
	    ++_telescopeHitsDelayed[channel].numer[iPlane];
	  }
	}
      }
    }
  }
}


void EventAnalyzer::CalculateAccidentalRates(PLTTelescope &telescope)
{
  unsigned channel  = telescope.Channel();
  if (telescope.NTracks() > 0) {
    //std::cout << telescope.NTracks() << std::endl;
    bool foundOneGoodTrack = false;
    bool hasNan = false;
    for (size_t itrack = 0; itrack < telescope.NTracks(); ++itrack) {
      PLTTrack *tr = telescope.Track(itrack);

      // apply track selection cuts
      float slopeX = tr->fTVX/tr->fTVZ;
      float slopeY = tr->fTVY/tr->fTVZ;
      //std::cout << tr->fTVX << ", " << tr->fTVY << ", " << tr->fTVZ << std::endl; 
      //std::cout << slopeX << ", " << slopeY << std::endl;
      if (isnan(slopeX) || isnan(slopeY)) {
	hasNan = true;
      }

      float dxSlope = fabs((slopeX - _meanSlopeX[channel])/_sigmaSlopeX[channel]);
      float dySlope = fabs((slopeY - _meanSlopeY[channel])/_sigmaSlopeY[channel]);
      float dxRes0 = fabs(tr->LResidualX(0) - _meanResidualX[channel][0])/_sigmaResidualX[channel][0];
      float dxRes1 = fabs(tr->LResidualX(1) - _meanResidualX[channel][1])/_sigmaResidualX[channel][1];
      float dxRes2 = fabs(tr->LResidualX(2) - _meanResidualX[channel][2])/_sigmaResidualX[channel][2];
      float dyRes0 = fabs(tr->LResidualY(0) - _meanResidualY[channel][0])/_sigmaResidualY[channel][0];
      float dyRes1 = fabs(tr->LResidualY(1) - _meanResidualY[channel][1])/_sigmaResidualY[channel][1];
      float dyRes2 = fabs(tr->LResidualY(2) - _meanResidualY[channel][2])/_sigmaResidualY[channel][2];


      //cout << "channel " << channel << ": " << dxSlope << ", " << dySlope 
      //     << " :: " << _accidentalRates[channel].second << endl;

      if (
	  //sqrt(pow(dxSlope,2) + pow(dySlope,2)) < 5.
                    dxSlope < 5. 
                    && dySlope < 5.
                    && dxRes0 < 5. && dxRes1 < 5. && dxRes2 < 5.  
                    && dyRes0 < 5. && dyRes1 < 5. && dyRes2 < 5.
	  ) {
	foundOneGoodTrack = true;
	//++_accidentalRates[channel].first; // good tracks
      } //else {
      //std::cout << slopeX << ", " << _meanSlopeX[channel] << ", " << _sigmaSlopeX[channel] << std::endl;
      //std::cout << slopeY << ", " << _meanSlopeY[channel] << ", " << _sigmaSlopeY[channel] << std::endl;
      //   std:: cout << dxSlope << ", " << dySlope << std::endl << std::endl;
      ///   ++_accidentalRates[channel].second; //bad tracks
      //}

      // record three-hit track slopes
      pair<float, float> slopes;
      slopes.first  = tr->fTVX/tr->fTVZ;
      slopes.second = tr->fTVY/tr->fTVZ;
      _threeHitTrackSlopes[channel].push_back(slopes);
      //break;
      //cout << "channel " << channel << ": " << slopes.first << " :: " << slopes.second << endl;
            
    }
    //std::cout << hasNan << ", " << foundOneGoodTrack << std::endl;
    if (!hasNan) {
      if (foundOneGoodTrack) 
	++_accidentalRates[channel].first; // good tracks
      else
	++_accidentalRates[channel].second; // bad tracks
    }
  }
}

vector<float> EventAnalyzer::GetTelescopeEfficiency(int channel)
{
  EffCounter planeCounts        = _telescopeHits[channel];
  EffCounter planeCountsDelayed = _telescopeHitsDelayed[channel];
  vector<float> eff (3, 1);
  for (unsigned i = 0; i < 3; ++i) {
    if (planeCounts.denom[i] - planeCountsDelayed.denom[i] > 0.) {
      eff[i]  = (planeCounts.numer[i] - planeCountsDelayed.numer[i]);
      eff[i] /= (planeCounts.denom[i] - planeCountsDelayed.denom[i]);
      //eff[i] = planeCounts.numer[i]/planeCounts.denom[i];
    }
  }
  return eff;
}

float EventAnalyzer::GetTelescopeAccidentals(int channel)
{
  float fakes = 0.;
  if (_accidentalRates[channel].second > 0) {
    fakes = float(_accidentalRates[channel].second)/(_accidentalRates[channel].first + _accidentalRates[channel].second);
    //std::cout << "Good " << _accidentalRates[channel].first << "All " << _accidentalRates[channel].first + _accidentalRates[channel].second << std::endl;
  }
  return fakes;
}

float EventAnalyzer::GetZeroCounting(int channel)
{
  float rate = 0.;
  if (_bxCounter > 0.) {
    rate = float(_accidentalRates[channel].first)/_bxCounter;
  } 
  return rate;
}

vector<pair<float, float> > EventAnalyzer::GetSlopes(int channel)
{
    vector<pair<float, float> > outSlopes;
    outSlopes = _threeHitTrackSlopes[channel];
    _threeHitTrackSlopes[channel].clear();
    return outSlopes;
}

void EventAnalyzer::ClearCounters()
{
  const unsigned validChannels[] = {2, 4, 5, 8, 10, 11, 13, 14, 16, 17, 19, 20};
  const unsigned nChannels = sizeof(validChannels) / sizeof(validChannels[0]);
  unsigned channel;
  for (unsigned i = 0; i < nChannels; ++i) {
    channel = validChannels[i];
    _telescopeHits[channel]        = EffCounter();
    _telescopeHitsDelayed[channel] = EffCounter();
    _accidentalRates[channel]      = make_pair(0, 0);
    _bxCounter                      = 0;
  }
}

