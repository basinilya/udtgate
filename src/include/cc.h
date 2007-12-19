/*****************************************************************************
DISCLAIMER: The algorithms implemented using UDT/CCC in this file may be
modified. These modifications may NOT necessarily reflect the view of
the algorithms' original authors.

Written by: Yunhong Gu <gu@lac.uic.edu>, last updated on July 15, 2007.
*****************************************************************************/

#ifndef WIN32
   #include <sys/time.h>
   #include <time.h>
#endif

#include <cmath>
#include <vector>
#include <algorithm>

#include <ccc.h>
#include <udt.h>


/*****************************************************************************
TCP congestion control
Reference: 
M. Allman, V. Paxson, W. Stevens (consultant), TCP Congestion Control, RFC 
2581, April 1999.

Note:
This base TCP control class can be used to derive new TCP variants, including
those implemented in this file: HighSpeed, Scalable, BiC, Vegas, and FAST.
*****************************************************************************/

class CTCP: public CCC
{
public:
   void init()
   {
      m_bSlowStart = true;
      m_issthresh = 83333;

      m_dPktSndPeriod = 0.0;
      m_dCWndSize = 2.0;

      setACKInterval(2);
      setRTO(1000000);
   }

   virtual void onACK(const int& ack)
   {
      if (ack == m_iLastACK)
      {
         if (3 == ++ m_iDupACKCount)
            DupACKAction();
         else if (m_iDupACKCount > 3)
            m_dCWndSize += 1.0;
         else
            ACKAction();
      }
      else
      {
         if (m_iDupACKCount >= 3)
            m_dCWndSize = m_issthresh;

         m_iLastACK = ack;
         m_iDupACKCount = 1;

         ACKAction();
      }
   }

   virtual void onTimeout()
   {
      m_issthresh = getPerfInfo()->pktFlightSize / 2;
      if (m_issthresh < 2)
         m_issthresh = 2;

      m_bSlowStart = true;
      m_dCWndSize = 2.0;
   }

protected:
   virtual void ACKAction()
   {
      if (m_bSlowStart)
      {
         m_dCWndSize += 1.0;

         if (m_dCWndSize >= m_issthresh)
            m_bSlowStart = false;
      }
      else
         m_dCWndSize += 1.0/m_dCWndSize;
   }

   virtual void DupACKAction()
   {
      m_bSlowStart = false;

      m_issthresh = getPerfInfo()->pktFlightSize / 2;
      if (m_issthresh < 2)
         m_issthresh = 2;

      m_dCWndSize = m_issthresh + 3;
   }

protected:
   int m_issthresh;
   bool m_bSlowStart;

   int m_iDupACKCount;
   int m_iLastACK;
};


/*****************************************************************************
Scalable TCP congestion control
Reference:
Tom Kelly, Scalable TCP: Improving Performance in Highspeed Wide Area 
Networks, Computer Communication Review, Vol. 33 No. 2 - April 2003
*****************************************************************************/

class CScalableTCP: public CTCP
{
protected:
   virtual void ACKAction()
   {
      if (m_dCWndSize <= 38.0)
         CTCP::ACKAction();
      else
      {
         if (m_bSlowStart)
            m_dCWndSize += 1.0;
         else
            m_dCWndSize += 0.01;
      }

      if (m_dCWndSize > m_iMaxCWndSize)
         m_dCWndSize = m_iMaxCWndSize;
   }

   virtual void DupACKAction()
   {
      if (m_dCWndSize <= 38.0)
         m_dCWndSize *= 0.5;
      else
         m_dCWndSize *= 0.875;

      if (m_dCWndSize < m_iMinCWndSize)
         m_dCWndSize = m_iMinCWndSize;
   }

private:
   static const int m_iMinCWndSize = 16;
   static const int m_iMaxCWndSize = 100000;

   static const int m_iCWndThresh = 38;
};


/*****************************************************************************
HighSpeed TCP congestion control
Reference:
Sally Floyd, HighSpeed TCP for Large Congestion Windows, RFC 3649, 
Experimental, December 2003
*****************************************************************************/

class CHSTCP: public CTCP
{
public:
   virtual void ACKAction()
   {
      m_dCWndSize += a(m_dCWndSize)/m_dCWndSize;
   }

   virtual void DupACKAction()
   {
      m_dCWndSize -= m_dCWndSize*b(m_dCWndSize);
   }

private:
   double a(double w)
   {
      return (w * w * 2.0 * b(w)) / ((2.0 - b(w)) * pow(w, 1.2) * 12.8);
   }

   double b(double w)
   {
      return (0.1 - 0.5) * (log(w) - log(38.)) / (log(83000.) - log(38.)) + 0.5;
   }

private:
   static const int m_iHighWnd = 83000;
   static const int m_iLowWnd = 38;
};


/*****************************************************************************
BiC TCP congestion control
Reference:
Lisong Xu, Khaled Harfoush, and Injong Rhee, "Binary Increase Congestion 
Control for Fast Long-Distance Networks", INFOCOM 2004.
*****************************************************************************/

class CBiCTCP: public CTCP
{
public:
   CBiCTCP()
   {
      m_dMaxWin = m_iDefaultMaxWin;
      m_dMinWin = m_dCWndSize;
      m_dTargetWin = (m_dMaxWin + m_dMinWin) / 2;

      m_dSSCWnd = 1.0;
      m_dSSTargetWin = m_dCWndSize + 1.0;
   }

protected:
   virtual void ACKAction()
   {
      if (m_dCWndSize < m_iLowWindow)
      {
         m_dCWndSize += 1/m_dCWndSize;
         return;
      }

      if (!m_bSlowStart)
      {
         if (m_dTargetWin - m_dCWndSize < m_iSMax)
            m_dCWndSize += (m_dTargetWin - m_dCWndSize)/m_dCWndSize;
         else
            m_dCWndSize += m_iSMax/m_dCWndSize;

         if (m_dMaxWin > m_dCWndSize)
         {
            m_dMinWin = m_dCWndSize;
            m_dTargetWin = (m_dMaxWin + m_dMinWin) / 2;
         }
         else
         {
            m_bSlowStart = true;
            m_dSSCWnd = 1.0;
            m_dSSTargetWin = m_dCWndSize + 1.0;
            m_dMaxWin = m_iDefaultMaxWin;
         }
      }
      else
      {
         m_dCWndSize += m_dSSCWnd/m_dCWndSize;
         if(m_dCWndSize >= m_dSSTargetWin)
         {
            m_dSSCWnd *= 2;
            m_dSSTargetWin = m_dCWndSize + m_dSSCWnd;
         }
         if(m_dSSCWnd >= m_iSMax)
            m_bSlowStart = false;
      }        
   }

   virtual void DupACKAction()
   {
      if (m_dCWndSize <= m_iLowWindow)
         m_dCWndSize *= 0.5;
      else
      {
         m_dPreMax = m_dMaxWin;
         m_dMaxWin = m_dCWndSize;
         m_dCWndSize *= 0.875;
         m_dMinWin = m_dCWndSize;

         if (m_dPreMax > m_dMaxWin)
         {
            m_dMaxWin = (m_dMaxWin + m_dMinWin) / 2;
            m_dTargetWin = (m_dMaxWin + m_dMinWin) / 2;
         }
      }
   }

private:
   static const int m_iLowWindow = 38;
   static const int m_iSMax = 32;
   static const int m_iSMin = 1;
   static const int m_iDefaultMaxWin = 1 << 29;

   double m_dMaxWin;
   double m_dMinWin;
   double m_dPreMax;
   double m_dTargetWin;
   double m_dSSCWnd;
   double m_dSSTargetWin;
};

/*****************************************************************************
TCP Westwood
reference:
http://www.cs.ucla.edu/NRL/hpi/tcpw/
*****************************************************************************/

// UDT timing utility class CTimer is reused here, defined in src/common.h
#include <common.h>

class CWestwood: public CTCP
{
public:
   CWestwood(): m_dBWE(1), m_dLastBWE(1), m_dBWESample(1), m_dLastBWESample(1)
   {
      m_LastACKTime = CTimer::getTime();
   }

   virtual void onACK(const int& ack)
   {
      uint64_t currtime = CTimer::getTime();

      m_dBWESample = double(ack - m_iLastACK) * 1000 / (currtime - m_LastACKTime);
      m_dBWE = 19.0/21.0 * m_dLastBWE + 1.0/21.0 * (m_dBWESample + m_dLastBWESample);

      m_LastACKTime = currtime;
      m_dLastBWE = m_dBWE;
      m_dLastBWESample = m_dBWESample;

      if (ack == m_iLastACK)
      {
         if (3 == ++ m_iDupACKCount)
         {
            m_bSlowStart = false;

            m_issthresh = int(ceil(getPerfInfo()->msRTT * m_dBWE));
            if (m_issthresh < 2)
            m_issthresh = 2;

            m_dCWndSize = m_issthresh + 3;
         }
         else if (m_iDupACKCount > 3)
            m_dCWndSize += 1.0;
         else
            ACKAction();
      }
      else
      {
         if (m_iDupACKCount >= 3)
            m_dCWndSize = m_issthresh;

         m_iLastACK = ack;
         m_iDupACKCount = 1;

         ACKAction();
      }
   }

   virtual void onTimeout()
   {
      m_issthresh = int(ceil(getPerfInfo()->msRTT * m_dBWE));
      if (m_issthresh < 2)
         m_issthresh = 2;

      m_bSlowStart = true;
      m_dCWndSize = 2.0;
   }

private:
   double m_dBWE, m_dLastBWE;
   double m_dBWESample, m_dLastBWESample;
   uint64_t m_LastACKTime;
};


/*****************************************************************************
TCP Vegas
Reference:
L. Brakmo, S. O'Malley, and L. Peterson. TCP Vegas: New techniques for 
congestion detection and avoidance. In Proceedings of the SIGCOMM '94 
Symposium (Aug. 1994) pages 24-35. 

Note:
This class can be used to derive new delay based approaches, e.g., FAST.
*****************************************************************************/

// A RTT calculating utility class is reused here, defined src/windows.h
#include <window.h>

class CVegas: public CTCP
{
public:
   CVegas()
   {
      m_iSSRound = 1;
      m_iRTT = 1000000;
      m_iBaseRTT = 1000000;
      m_LastCCTime = CTimer::getTime();
      m_iPktSent = 0;
      m_pAckWindow = new CACKWindow(100000);
   }

   ~CVegas()
   {
      delete m_pAckWindow; 
   }

   virtual void onACK(const int& seq)
   {
      double expected, actual, diff; //kbps

      int rtt = m_pAckWindow->acknowledge(seq, const_cast<int&>(seq));
      if (rtt > 0)
         m_iRTT = (m_iRTT * 15 + rtt) >> 4;

      uint64_t currtime = CTimer::getTime();
      if ((currtime - m_LastCCTime) < (uint64_t)m_iRTT)
         return;

      expected = m_dCWndSize * 1000.0 / m_iBaseRTT;
      actual = m_iPktSent * 1000.0 / (currtime - m_LastCCTime);
      diff = expected - actual;

      if (m_bSlowStart)
      {
         if (diff < gamma)
            m_bSlowStart = false;

         if (m_iSSRound & 1)
            m_dCWndSize *= 2;

         m_iSSRound ++;
      }
      else
      {
         if (diff < alpha)
            m_dCWndSize += 1.0;
         else if (diff > beta)
            m_dCWndSize -= 1.0;
      }

      m_LastCCTime = CTimer::getTime();
      m_iPktSent = 0;
      if (m_iBaseRTT > m_iRTT)
         m_iBaseRTT = m_iRTT;
   }

   virtual void onPktSent(const CPacket* pkt)
   {
      m_pAckWindow->store(pkt->m_iSeqNo, pkt->m_iSeqNo);
      m_iPktSent ++;
   }

   virtual void onTimeout()
   {
   }

protected:
   int m_iSSRound;

   int m_iRTT;
   int m_iBaseRTT;
   uint64_t m_LastCCTime;

   int m_iPktSent;

   static const int alpha = 30; //kbps
   static const int beta = 60;  //kbps
   static const int gamma = 30; //kbps

   CACKWindow* m_pAckWindow;
};


/*****************************************************************************
FAST TCP
Reference:
1. C. Jin, D. X. Wei and S. H. Low, "FAST TCP: motivation, architecture, 
   algorithms, performance", IEEE Infocom, March 2004
2. C. Jin, D. X. Wei and S. H. Low, FAST TCP for High-Speed Long-Distance 
   Networks, Internet Draft, draft-jwl-tcp-fast-01.txt,
   http://netlab.caltech.edu/pub/papers/draft-jwl-tcp-fast-01.txt

Note:
   Precision of RTT measurement may make great difference in the throughput
*****************************************************************************/

class CFAST: public CVegas
{
public:
   CFAST()
   {
      m_dOldWin = m_dCWndSize;
      m_iNumACK = 100000;
   }

   virtual void onACK(const int& ack)
   {
      if (ack == m_iLastACK)
      {
         if (3 == ++ m_iDupACKCount)
         {
            m_dCWndSize *= 0.875;
            return;
         }
      }
      else
      {
         if (m_iDupACKCount >= 3)
         {
//            m_dCWndSize = m_issthresh;
//            return;
         }

         m_iLastACK = ack;
         m_iDupACKCount = 1;
      }

      if (0 == (++ m_iACKCount % m_iNumACK))
         m_dCWndSize += m_iIncDec;

      int rtt = m_pAckWindow->acknowledge(ack, const_cast<int&>(ack));
      if (rtt > 0)
         m_iRTT = (m_iRTT * 7 + rtt) >> 3;

      uint64_t currtime = CTimer::getTime();
      if ((currtime - m_LastCCTime) < 2 * (uint64_t)m_iRTT)
         return;

      m_dNewWin = 0.5 * (m_dOldWin + (double(m_iBaseRTT) / m_iRTT) * m_dCWndSize + alpha);
      if (m_dNewWin > 2.0 * m_dCWndSize)
        m_dNewWin = 2.0 * m_dCWndSize;

      m_iNumACK = int(ceil(fabs(m_dCWndSize / (m_dNewWin - m_dCWndSize)) / 2.0));
      if (m_dNewWin > m_dCWndSize)
         m_iIncDec = 1;
      else
         m_iIncDec = -1;

      m_dOldWin = m_dCWndSize;

      m_LastCCTime = CTimer::getTime();
      m_iPktSent = 0;
      if (m_iBaseRTT > m_iRTT)
         m_iBaseRTT = m_iRTT;
   }

private:
   static const int alpha = 200;

   double m_dOldWin;
   double m_dNewWin;

   int m_iNumACK;
   int m_iIncDec;
   int m_iACKCount;
};


/*****************************************************************************
Reliable UDP Blast

Note:
This class demostrates the simplest control mechanism. The sending rate can
be set at any time by using setRate().
*****************************************************************************/

class CUDPBlast: public CCC
{
public:
   CUDPBlast()
   {
      m_dPktSndPeriod = 1000000; 
      m_dCWndSize = 83333.0; 
   }

public:
   void setRate(int mbps)
   {
      m_dPktSndPeriod = (m_iMSS * 8.0) / mbps;
   }
};
