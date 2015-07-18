////////////////////////////////////////////////////////////////////////////////
///
/// 'FIFOSamplePipe' : An abstract base class for classes that manipulate sound
/// samples by operating like a first-in-first-out pipe: New samples are fed
/// into one end of the pipe with the 'putSamples' function, and the processed
/// samples are received from the other end with the 'receiveSamples' function.
///
/// 'FIFOProcessor' : A base class for classes the do signal processing with 
/// the samples while operating like a first-in-first-out pipe. When samples
/// are input with the 'putSamples' function, the class processes them
/// and moves the processed samples to the given 'output' pipe object, which
/// may be either another processing stage, or a fifo sample buffer object.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-06-13 15:29:53 -0400 (Wed, 13 Jun 2012) $
// File revision : $Revision: 4 $
//
// $Id: FIFOSamplePipe.h 143 2012-06-13 19:29:53Z oparviai $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#ifndef FIFOSamplePipe_H
#define FIFOSamplePipe_H

#include <assert.h>
#include <stdlib.h>
#include "STTypes.h"

namespace soundtouch
{
/// Abstract base class for FIFO (first-in-first-out) sample processing classes.
class FIFOSamplePipe
{
public:
    // virtual default destructor
    virtual ~FIFOSamplePipe() {}
    /// Returns a pointer to the beginning of the output samples. 
    /// This function is provided for accessing the output samples directly. 
    /// Please be careful for not to corrupt the book-keeping!
    ///
    /// When using this function to output samples, also remember to 'remove' the
    /// output samples from the buffer by calling the 
    /// 'receiveSamples(size)' function
    virtual CSAMPLE *begin() = 0;
    /// Adds 'size' pcs of samples from the 'samples' memory position to
    /// the sample buffer.
    virtual void putSamples(const CSAMPLE *samples,  ///< Pointer to samples.
                            uint size             ///< Number of samples to insert.
                            ) = 0;
    // Moves samples from the 'other' pipe instance to this instance.
    void moveSamples(FIFOSamplePipe &other  ///< Other pipe instance where from the receive the data.
         ){
        int oNumSamples = other.size();
        putSamples(other.begin(), oNumSamples);
        other.receiveSamples(oNumSamples);
    };
    /// Output samples from beginning of the sample buffer. Copies requested samples to 
    /// output buffer and removes them from the sample buffer. If there are less than 
    /// 'numsample' samples in the buffer, returns all that available.
    ///
    /// \return Number of samples returned.
    virtual uint receiveSamples(CSAMPLE *output, ///< Buffer where to copy output samples.
                                uint maxSamples                 ///< How many samples to receive at max.
                                ) = 0;
    /// Adjusts book-keeping so that given number of samples are removed from beginning of the 
    /// sample buffer without copying them anywhere. 
    ///
    /// Used to reduce the number of samples in the buffer when accessing the sample buffer directly
    /// with 'begin' function.
    virtual uint receiveSamples(uint maxSamples   ///< Remove this many samples from the beginning of pipe.
                                ) = 0;
    /// Returns number of samples currently available.
    virtual uint size() const = 0;
    // Returns nonzero if there aren't any samples available for outputting.
    virtual bool empty() const = 0;
    /// Clears all the samples.
    virtual void clear() = 0;
    /// allow trimming (downwards) amount of samples in pipeline.
    /// Returns adjusted amount of samples
    virtual uint resize(uint size) = 0;

};



/// Base-class for sound processing routines working in FIFO principle. With this base 
/// class it's easy to implement sound processing stages that can be chained together,
/// so that samples that are fed into beginning of the pipe automatically go through 
/// all the processing stages.
///
/// When samples are input to this class, they're first processed and then put to 
/// the FIFO pipe that's defined as output of this class. This output pipe can be
/// either other processing stage or a FIFO sample buffer.
class FIFOProcessor :public FIFOSamplePipe
{
protected:
    /// Internal pipe where processed samples are put.
    FIFOSamplePipe *output;
    /// Sets output pipe.
    void setOutPipe(FIFOSamplePipe *pOutput){
        assert(output == NULL);
        assert(pOutput != NULL);
        output = pOutput;
    }
    /// Constructor. Doesn't define output pipe; it has to be set be 
    /// 'setOutPipe' function.
    FIFOProcessor()
      :output(nullptr){}
    /// Constructor. Configures output pipe.
    FIFOProcessor(FIFOSamplePipe *pOutput):output(pOutput){}
    /// Destructor.
    virtual ~FIFOProcessor(){}
    /// Returns a pointer to the beginning of the output samples. 
    /// This function is provided for accessing the output samples directly. 
    /// Please be careful for not to corrupt the book-keeping!
    ///
    /// When using this function to output samples, also remember to 'remove' the
    /// output samples from the buffer by calling the 
    /// 'receiveSamples(size)' function
    virtual CSAMPLE *begin() {return output->begin();}
public:
    /// Output samples from beginning of the sample buffer. Copies requested samples to 
    /// output buffer and removes them from the sample buffer. If there are less than 
    /// 'numsample' samples in the buffer, returns all that available.
    ///
    /// \return Number of samples returned.
    virtual uint receiveSamples(CSAMPLE *outBuffer, ///< Buffer where to copy output samples.
                                uint maxSamples                    ///< How many samples to receive at max.
                                ){
        return output->receiveSamples(outBuffer, maxSamples);
    }
    /// Adjusts book-keeping so that given number of samples are removed from beginning of the 
    /// sample buffer without copying them anywhere. 
    ///
    /// Used to reduce the number of samples in the buffer when accessing the sample buffer directly
    /// with 'begin' function.
    virtual uint receiveSamples(uint maxSamples   ///< Remove this many samples from the beginning of pipe.
                                ){
        return output->receiveSamples(maxSamples);
    }
    /// Returns number of samples currently available.
    virtual uint size() const{return output->size();}
    /// Returns nonzero if there aren't any samples available for outputting.
    virtual bool empty() const{return output->empty();}
    /// allow trimming (downwards) amount of samples in pipeline.
    /// Returns adjusted amount of samples
    virtual uint resize(uint size){return output->resize(size);}
};
}
#endif
