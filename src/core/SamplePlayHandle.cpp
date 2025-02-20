/*
 * SamplePlayHandle.cpp - implementation of class SamplePlayHandle
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "SamplePlayHandle.h"
#include "AudioEngine.h"
#include "AudioPort.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "lmms_constants.h"
#include "PatternTrack.h"
#include "SampleClip.h"



SamplePlayHandle::SamplePlayHandle( SampleBuffer* sampleBuffer , bool ownAudioPort ) :
	PlayHandle( TypeSamplePlayHandle ),
	m_sampleBuffer( sharedObject::ref( sampleBuffer ) ),
	m_doneMayReturnTrue( true ),
	m_frame( 0 ),
	m_ownAudioPort( ownAudioPort ),
	m_defaultVolumeModel( DefaultVolume, MinVolume, MaxVolume, 1 ),
	m_volumeModel( &m_defaultVolumeModel ),
	m_track( nullptr ),
	m_patternTrack( nullptr )
{
	if (ownAudioPort)
	{
		setAudioPort( new AudioPort( "SamplePlayHandle", false ) );
	}
}




SamplePlayHandle::SamplePlayHandle( const QString& sampleFile ) :
	SamplePlayHandle( new SampleBuffer( sampleFile ) , true)
{
	sharedObject::unref( m_sampleBuffer );
}




SamplePlayHandle::SamplePlayHandle( SampleClip* clip ) :
	SamplePlayHandle( clip->sampleBuffer() , false)
{
	m_track = clip->getTrack();
	setAudioPort( ( (SampleTrack *)clip->getTrack() )->audioPort() );
}




SamplePlayHandle::~SamplePlayHandle()
{
	sharedObject::unref( m_sampleBuffer );
	if( m_ownAudioPort )
	{
		delete audioPort();
	}
}




void SamplePlayHandle::play( sampleFrame * buffer )
{
	const fpp_t fpp = Engine::audioEngine()->framesPerPeriod();
	//play( 0, _try_parallelizing );
	if( framesDone() >= totalFrames() )
	{
		memset( buffer, 0, sizeof( sampleFrame ) * fpp );
		return;
	}

	sampleFrame * workingBuffer = buffer;
	f_cnt_t frames = fpp;

	// apply offset for the first period
	if( framesDone() == 0 )
	{
		memset( buffer, 0, sizeof( sampleFrame ) * offset() );
		workingBuffer += offset();
		frames -= offset();
	}

	if( !( m_track && m_track->isMuted() )
				&& !(m_patternTrack && m_patternTrack->isMuted()))
	{
/*		stereoVolumeVector v =
			{ { m_volumeModel->value() / DefaultVolume,
				m_volumeModel->value() / DefaultVolume } };*/
		// SamplePlayHandle always plays the sample at its original pitch;
		// it is used only for previews, SampleTracks and the metronome.
		if (!m_sampleBuffer->play(workingBuffer, &m_state, frames, DefaultBaseFreq))
		{
			memset(workingBuffer, 0, frames * sizeof(sampleFrame));
		}
	}

	m_frame += frames;
}




bool SamplePlayHandle::isFinished() const
{
	return framesDone() >= totalFrames() && m_doneMayReturnTrue == true;
}




bool SamplePlayHandle::isFromTrack( const Track * _track ) const
{
	return m_track == _track || m_patternTrack == _track;
}




f_cnt_t SamplePlayHandle::totalFrames() const
{
	return ( m_sampleBuffer->endFrame() - m_sampleBuffer->startFrame() ) *
			( Engine::audioEngine()->processingSampleRate() / m_sampleBuffer->sampleRate() );
}




