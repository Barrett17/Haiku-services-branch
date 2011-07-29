/*
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */

#include "AVFormatReader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <new>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <ByteOrder.h>
#include <DataIO.h>
#include <MediaDefs.h>
#include <MediaFormats.h>

extern "C" {
	#include "avcodec.h"
	#include "avformat.h"
}

#include "DemuxerTable.h"
#include "gfx_util.h"


//#define TRACE_AVFORMAT_READER
#ifdef TRACE_AVFORMAT_READER
#	define TRACE printf
#	define TRACE_IO(a...)
#	define TRACE_SEEK(a...) printf(a)
#	define TRACE_FIND(a...)
#	define TRACE_PACKET(a...)
#else
#	define TRACE(a...)
#	define TRACE_IO(a...)
#	define TRACE_SEEK(a...)
#	define TRACE_FIND(a...)
#	define TRACE_PACKET(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)


static const int64 kNoPTSValue = 0x8000000000000000LL;
	// NOTE: For some reasons, I have trouble with the avcodec.h define:
	// #define AV_NOPTS_VALUE          INT64_C(0x8000000000000000)
	// INT64_C is not defined here.


static uint32
avformat_to_beos_format(SampleFormat format)
{
	switch (format) {
		case SAMPLE_FMT_U8: return media_raw_audio_format::B_AUDIO_UCHAR;
		case SAMPLE_FMT_S16: return media_raw_audio_format::B_AUDIO_SHORT;
		case SAMPLE_FMT_S32: return media_raw_audio_format::B_AUDIO_INT;
		case SAMPLE_FMT_FLT: return media_raw_audio_format::B_AUDIO_FLOAT;
		case SAMPLE_FMT_DBL: return media_raw_audio_format::B_AUDIO_DOUBLE;
		default:
			break;
	}
	return 0;
}


static uint32
avformat_to_beos_byte_order(SampleFormat format)
{
	// TODO: Huh?
	return B_MEDIA_HOST_ENDIAN;
}


static void
avmetadata_to_message(AVMetadata* metaData, BMessage* message)
{
	if (metaData == NULL)
		return;

	AVMetadataTag* tag = NULL;
	while ((tag = av_metadata_get(metaData, "", tag,
		AV_METADATA_IGNORE_SUFFIX))) {
		// convert tag keys into something more meaningful using the names from
		// id3v2.c
		if (strcmp(tag->key, "TALB") == 0 || strcmp(tag->key, "TAL") == 0)
			message->AddString("album", tag->value);
		else if (strcmp(tag->key, "TCOM") == 0)
			message->AddString("composer", tag->value);
		else if (strcmp(tag->key, "TCON") == 0 || strcmp(tag->key, "TCO") == 0)
			message->AddString("genre", tag->value);
		else if (strcmp(tag->key, "TCOP") == 0)
			message->AddString("copyright", tag->value);
		else if (strcmp(tag->key, "TDRL") == 0 || strcmp(tag->key, "TDRC") == 0)
			message->AddString("date", tag->value);
		else if (strcmp(tag->key, "TENC") == 0 || strcmp(tag->key, "TEN") == 0)
			message->AddString("encoded_by", tag->value);
		else if (strcmp(tag->key, "TIT2") == 0 || strcmp(tag->key, "TT2") == 0)
			message->AddString("title", tag->value);
		else if (strcmp(tag->key, "TLAN") == 0)
			message->AddString("language", tag->value);
		else if (strcmp(tag->key, "TPE1") == 0 || strcmp(tag->key, "TP1") == 0)
			message->AddString("artist", tag->value);
		else if (strcmp(tag->key, "TPE2") == 0 || strcmp(tag->key, "TP2") == 0)
			message->AddString("album_artist", tag->value);
		else if (strcmp(tag->key, "TPE3") == 0 || strcmp(tag->key, "TP3") == 0)
			message->AddString("performer", tag->value);
		else if (strcmp(tag->key, "TPOS") == 0)
			message->AddString("disc", tag->value);
		else if (strcmp(tag->key, "TPUB") == 0)
			message->AddString("publisher", tag->value);
		else if (strcmp(tag->key, "TRCK") == 0 || strcmp(tag->key, "TRK") == 0)
			message->AddString("track", tag->value);
		else if (strcmp(tag->key, "TSOA") == 0)
			message->AddString("album-sort", tag->value);
		else if (strcmp(tag->key, "TSOP") == 0)
			message->AddString("artist-sort", tag->value);
		else if (strcmp(tag->key, "TSOT") == 0)
			message->AddString("title-sort", tag->value);
		else if (strcmp(tag->key, "TSSE") == 0)
			message->AddString("encoder", tag->value);
		else if (strcmp(tag->key, "TYER") == 0)
			message->AddString("year", tag->value);
		else
			message->AddString(tag->key, tag->value);
	}
}


// #pragma mark - StreamBase


class StreamBase {
public:
								StreamBase(BPositionIO* source,
									BLocker* sourceLock, BLocker* streamLock);
	virtual						~StreamBase();

	// Init an indivual AVFormatContext
			status_t			Open();

	// Setup this stream to point to the AVStream at the given streamIndex.
	virtual	status_t			Init(int32 streamIndex);

	inline	const AVFormatContext* Context() const
									{ return fContext; }
			int32				Index() const;
			int32				CountStreams() const;
			int32				StreamIndexFor(int32 virtualIndex) const;
	inline	int32				VirtualIndex() const
									{ return fVirtualIndex; }

			double				FrameRate() const;
			bigtime_t			Duration() const;

	virtual	status_t			Seek(uint32 flags, int64* frame,
									bigtime_t* time);

			status_t			GetNextChunk(const void** chunkBuffer,
									size_t* chunkSize,
									media_header* mediaHeader);

protected:
	// I/O hooks for libavformat, cookie will be a Stream instance.
	// Since multiple StreamCookies use the same BPositionIO source, they
	// maintain the position individually, and may need to seek the source
	// if it does not match anymore in _Read().
	// TODO: This concept prevents the use of a plain BDataIO that is not
	// seekable. There is a version of AVFormatReader in the SVN history
	// which implements packet buffering for other streams when reading
	// packets. To support non-seekable network streams for example, this
	// code should be resurrected. It will make handling seekable streams,
	// especially from different threads that read from totally independent
	// positions in the stream (aggressive pre-buffering perhaps), a lot
	// more difficult with potentially large memory overhead.
	static	int					_Read(void* cookie, uint8* buffer,
									int bufferSize);
	static	off_t				_Seek(void* cookie, off_t offset, int whence);

			status_t			_NextPacket(bool reuse);

			int64_t				_ConvertToStreamTimeBase(bigtime_t time) const;
			bigtime_t			_ConvertFromStreamTimeBase(int64_t time) const;

protected:
			BPositionIO*		fSource;
			off_t				fPosition;
			// Since different threads may read from the source,
			// we need to protect the file position and I/O by a lock.
			BLocker*			fSourceLock;

			BLocker*			fStreamLock;

			AVFormatContext*	fContext;
			AVStream*			fStream;
			int32				fVirtualIndex;

			media_format		fFormat;

			ByteIOContext		fIOContext;

			AVPacket			fPacket;
			bool				fReusePacket;

			bool				fSeekByBytes;
			bool				fStreamBuildsIndexWhileReading;
};


StreamBase::StreamBase(BPositionIO* source, BLocker* sourceLock,
		BLocker* streamLock)
	:
	fSource(source),
	fPosition(0),
	fSourceLock(sourceLock),

	fStreamLock(streamLock),

	fContext(NULL),
	fStream(NULL),
	fVirtualIndex(-1),

	fReusePacket(false),

	fSeekByBytes(false),
	fStreamBuildsIndexWhileReading(false)
{
	// NOTE: Don't use streamLock here, it may not yet be initialized!

	fIOContext.buffer = NULL;
	av_new_packet(&fPacket, 0);
	memset(&fFormat, 0, sizeof(media_format));
}


StreamBase::~StreamBase()
{
	av_free(fIOContext.buffer);
	av_free_packet(&fPacket);
	av_free(fContext);
}


status_t
StreamBase::Open()
{
	BAutolock _(fStreamLock);

	// Init probing data
	size_t bufferSize = 32768;
	uint8* buffer = static_cast<uint8*>(av_malloc(bufferSize));
	if (buffer == NULL)
		return B_NO_MEMORY;

	size_t probeSize = 2048;
	AVProbeData probeData;
	probeData.filename = "";
	probeData.buf = buffer;
	probeData.buf_size = probeSize;

	// Read a bit of the input...
	// NOTE: Even if other streams have already read from the source,
	// it is ok to not seek first, since our fPosition is 0, so the necessary
	// seek will happen automatically in _Read().
	if (_Read(this, buffer, probeSize) != (ssize_t)probeSize) {
		av_free(buffer);
		return B_IO_ERROR;
	}
	// ...and seek back to the beginning of the file. This is important
	// since libavformat will assume the stream to be at offset 0, the
	// probe data is not reused.
	_Seek(this, 0, SEEK_SET);

	// Probe the input format
	AVInputFormat* inputFormat = av_probe_input_format(&probeData, 1);

	if (inputFormat == NULL) {
		TRACE("StreamBase::Open() - av_probe_input_format() failed!\n");
		av_free(buffer);
		return B_NOT_SUPPORTED;
	}

	TRACE("StreamBase::Open() - "
		"av_probe_input_format(): %s\n", inputFormat->name);
	TRACE("  flags:%s%s%s%s%s\n",
		(inputFormat->flags & AVFMT_GLOBALHEADER) ? " AVFMT_GLOBALHEADER" : "",
		(inputFormat->flags & AVFMT_NOTIMESTAMPS) ? " AVFMT_NOTIMESTAMPS" : "",
		(inputFormat->flags & AVFMT_GENERIC_INDEX) ? " AVFMT_GENERIC_INDEX" : "",
		(inputFormat->flags & AVFMT_TS_DISCONT) ? " AVFMT_TS_DISCONT" : "",
		(inputFormat->flags & AVFMT_VARIABLE_FPS) ? " AVFMT_VARIABLE_FPS" : ""
	);

	// Init I/O context with buffer and hook functions, pass ourself as
	// cookie.
	memset(buffer, 0, bufferSize);
	if (init_put_byte(&fIOContext, buffer, bufferSize, 0, this,
			_Read, 0, _Seek) != 0) {
		TRACE("StreamBase::Open() - init_put_byte() failed!\n");
		return B_ERROR;
	}

	// Initialize our context.
	if (av_open_input_stream(&fContext, &fIOContext, "", inputFormat,
			NULL) < 0) {
		TRACE("StreamBase::Open() - av_open_input_stream() failed!\n");
		return B_NOT_SUPPORTED;
	}

	// Retrieve stream information
	if (av_find_stream_info(fContext) < 0) {
		TRACE("StreamBase::Open() - av_find_stream_info() failed!\n");
		return B_NOT_SUPPORTED;
	}

	fSeekByBytes = (inputFormat->flags & AVFMT_TS_DISCONT) != 0;
	fStreamBuildsIndexWhileReading
		= (inputFormat->flags & AVFMT_GENERIC_INDEX) != 0
			|| fSeekByBytes;

	TRACE("StreamBase::Open() - "
		"av_find_stream_info() success! Seeking by bytes: %d\n",
		fSeekByBytes);

	return B_OK;
}


status_t
StreamBase::Init(int32 virtualIndex)
{
	BAutolock _(fStreamLock);

	TRACE("StreamBase::Init(%ld)\n", virtualIndex);

	if (fContext == NULL)
		return B_NO_INIT;

	int32 streamIndex = StreamIndexFor(virtualIndex);
	if (streamIndex < 0) {
		TRACE("  bad stream index!\n");
		return B_BAD_INDEX;
	}

	TRACE("  context stream index: %ld\n", streamIndex);

	// We need to remember the virtual index so that
	// AVFormatReader::FreeCookie() can clear the correct stream entry.
	fVirtualIndex = virtualIndex;

	// Make us point to the AVStream at streamIndex
	fStream = fContext->streams[streamIndex];

// NOTE: Discarding other streams works for most, but not all containers,
// for example it does not work for the ASF demuxer. Since I don't know what
// other demuxer it breaks, let's just keep reading packets for unwanted
// streams, it just makes the _GetNextPacket() function slightly less
// efficient.
//	// Discard all other streams
//	for (unsigned i = 0; i < fContext->nb_streams; i++) {
//		if (i != (unsigned)streamIndex)
//			fContext->streams[i]->discard = AVDISCARD_ALL;
//	}

	return B_OK;
}


int32
StreamBase::Index() const
{
	if (fStream != NULL)
		return fStream->index;
	return -1;
}


int32
StreamBase::CountStreams() const
{
	// Figure out the stream count. If the context has "AVPrograms", use
	// the first program (for now).
	// TODO: To support "programs" properly, the BMediaFile/Track API should
	// be extended accordingly. I guess programs are like TV channels in the
	// same satilite transport stream. Maybe call them "TrackGroups".
	if (fContext->nb_programs > 0) {
		// See libavformat/utils.c:dump_format()
		return fContext->programs[0]->nb_stream_indexes;
	}
	return fContext->nb_streams;
}


int32
StreamBase::StreamIndexFor(int32 virtualIndex) const
{
	// NOTE: See CountStreams()
	if (fContext->nb_programs > 0) {
		const AVProgram* program = fContext->programs[0];
		if (virtualIndex >= 0
			&& virtualIndex < (int32)program->nb_stream_indexes) {
			return program->stream_index[virtualIndex];
		}
	} else {
		if (virtualIndex >= 0 && virtualIndex < (int32)fContext->nb_streams)
			return virtualIndex;
	}
	return -1;
}


double
StreamBase::FrameRate() const
{
	// TODO: Find a way to always calculate a correct frame rate...
	double frameRate = 1.0;
	switch (fStream->codec->codec_type) {
		case CODEC_TYPE_AUDIO:
			frameRate = (double)fStream->codec->sample_rate;
			break;
		case CODEC_TYPE_VIDEO:
			if (fStream->avg_frame_rate.den && fStream->avg_frame_rate.num)
				frameRate = av_q2d(fStream->avg_frame_rate);
			else if (fStream->r_frame_rate.den && fStream->r_frame_rate.num)
				frameRate = av_q2d(fStream->r_frame_rate);
			else if (fStream->time_base.den && fStream->time_base.num)
				frameRate = 1 / av_q2d(fStream->time_base);
			else if (fStream->codec->time_base.den
				&& fStream->codec->time_base.num) {
				frameRate = 1 / av_q2d(fStream->codec->time_base);
			}

			// TODO: Fix up interlaced video for real
			if (frameRate == 50.0f)
				frameRate = 25.0f;
			break;
		default:
			break;
	}
	if (frameRate <= 0.0)
		frameRate = 1.0;
	return frameRate;
}


bigtime_t
StreamBase::Duration() const
{
	// TODO: This is not working correctly for all stream types...
	// It seems that the calculations here are correct, because they work
	// for a couple of streams and are in line with the documentation, but
	// unfortunately, libavformat itself seems to set the time_base and
	// duration wrongly sometimes. :-(
	if ((int64)fStream->duration != kNoPTSValue)
		return _ConvertFromStreamTimeBase(fStream->duration);
	else if ((int64)fContext->duration != kNoPTSValue)
		return (bigtime_t)fContext->duration;

	return 0;
}


status_t
StreamBase::Seek(uint32 flags, int64* frame, bigtime_t* time)
{
	BAutolock _(fStreamLock);

	if (fContext == NULL || fStream == NULL)
		return B_NO_INIT;

	TRACE_SEEK("StreamBase::Seek(%ld,%s%s%s%s, %lld, "
		"%lld)\n", VirtualIndex(),
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD)
			? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD)
			? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		*frame, *time);

	double frameRate = FrameRate();
	if ((flags & B_MEDIA_SEEK_TO_FRAME) != 0) {
		// Seeking is always based on time, initialize it when client seeks
		// based on frame.
		*time = (bigtime_t)(*frame * 1000000.0 / frameRate + 0.5);
	}

	int64_t timeStamp = *time;

	int searchFlags = AVSEEK_FLAG_BACKWARD;
	if ((flags & B_MEDIA_SEEK_CLOSEST_FORWARD) != 0)
		searchFlags = 0;

	if (fSeekByBytes) {
		searchFlags |= AVSEEK_FLAG_BYTE;

		BAutolock _(fSourceLock);
		int64_t fileSize;
		if (fSource->GetSize(&fileSize) != B_OK)
			return B_NOT_SUPPORTED;
		int64_t duration = Duration();
		if (duration == 0)
			return B_NOT_SUPPORTED;

		timeStamp = int64_t(fileSize * ((double)timeStamp / duration));
		if ((flags & B_MEDIA_SEEK_CLOSEST_BACKWARD) != 0) {
			timeStamp -= 65536;
			if (timeStamp < 0)
				timeStamp = 0;
		}

		bool seekAgain = true;
		bool seekForward = true;
		bigtime_t lastFoundTime = -1;
		int64_t closestTimeStampBackwards = -1;
		while (seekAgain) {
			if (avformat_seek_file(fContext, -1, INT64_MIN, timeStamp,
				INT64_MAX, searchFlags) < 0) {
				TRACE("  avformat_seek_file() (by bytes) failed.\n");
				return B_ERROR;
			}
			seekAgain = false;

			// Our last packet is toast in any case. Read the next one so we
			// know where we really seeked.
			fReusePacket = false;
			if (_NextPacket(true) == B_OK) {
				while (fPacket.pts == kNoPTSValue) {
					fReusePacket = false;
					if (_NextPacket(true) != B_OK)
						return B_ERROR;
				}
				if (fPacket.pos >= 0)
					timeStamp = fPacket.pos;
				bigtime_t foundTime
					= _ConvertFromStreamTimeBase(fPacket.pts);
				if (foundTime != lastFoundTime) {
					lastFoundTime = foundTime;
					if (foundTime > *time) {
						if (closestTimeStampBackwards >= 0) {
							timeStamp = closestTimeStampBackwards;
							seekAgain = true;
							seekForward = false;
							continue;
						}
						int64_t diff = int64_t(fileSize
							* ((double)(foundTime - *time) / (2 * duration)));
						if (diff < 8192)
							break;
						timeStamp -= diff;
						TRACE_SEEK("  need to seek back (%lld) (time: %.2f "
							"-> %.2f)\n", timeStamp, *time / 1000000.0,
							foundTime / 1000000.0);
						if (timeStamp < 0)
							foundTime = 0;
						else {
							seekAgain = true;
							continue;
						}
					} else if (seekForward && foundTime < *time - 100000) {
						closestTimeStampBackwards = timeStamp;
						int64_t diff = int64_t(fileSize
							* ((double)(*time - foundTime) / (2 * duration)));
						if (diff < 8192)
							break;
						timeStamp += diff;
						TRACE_SEEK("  need to seek forward (%lld) (time: "
							"%.2f -> %.2f)\n", timeStamp, *time / 1000000.0,
							foundTime / 1000000.0);
						if (timeStamp > duration)
							foundTime = duration;
						else {
							seekAgain = true;
							continue;
						}
					}
				}
				TRACE_SEEK("  found time: %lld -> %lld (%.2f)\n", *time,
					foundTime, foundTime / 1000000.0);
				*time = foundTime;
				*frame = (uint64)(*time * frameRate / 1000000LL + 0.5);
				TRACE_SEEK("  seeked frame: %lld\n", *frame);
			} else {
				TRACE_SEEK("  _NextPacket() failed!\n");
				return B_ERROR;
			}
		}
	} else {
		// We may not get a PTS from the next packet after seeking, so
		// we try to get an expected time from the index.
		int64_t streamTimeStamp = _ConvertToStreamTimeBase(*time);
		int index = av_index_search_timestamp(fStream, streamTimeStamp,
			searchFlags);
		if (index < 0) {
			TRACE("  av_index_search_timestamp() failed\n");
		} else {
			if (index > 0) {
				const AVIndexEntry& entry = fStream->index_entries[index];
				streamTimeStamp = entry.timestamp;
			} else {
				// Some demuxers use the first index entry to store some
				// other information, like the total playing time for example.
				// Assume the timeStamp of the first entry is alays 0.
				// TODO: Handle start-time offset?
				streamTimeStamp = 0;
			}
			bigtime_t foundTime = _ConvertFromStreamTimeBase(streamTimeStamp);
			bigtime_t timeDiff = foundTime > *time
				? foundTime - *time : *time - foundTime;

			if (timeDiff > 1000000
				&& (fStreamBuildsIndexWhileReading
					|| index == fStream->nb_index_entries - 1)) {
				// If the stream is building the index on the fly while parsing
				// it, we only have entries in the index for positions already
				// decoded, i.e. we cannot seek into the future. In that case,
				// just assume that we can seek where we want and leave
				// time/frame unmodified. Since successfully seeking one time
				// will generate index entries for the seeked to position, we
				// need to remember this in fStreamBuildsIndexWhileReading,
				// since when seeking back there will be later index entries,
				// but we still want to ignore the found entry.
				fStreamBuildsIndexWhileReading = true;
				TRACE_SEEK("  Not trusting generic index entry. "
					"(Current count: %d)\n", fStream->nb_index_entries);
			} else {
				// If we found a reasonably time, write it into *time.
				// After seeking, we will try to read the sought time from
				// the next packet. If the packet has no PTS value, we may
				// still have a more accurate time from the index lookup.
				*time = foundTime;
			}
		}

		if (avformat_seek_file(fContext, -1, INT64_MIN, timeStamp, INT64_MAX,
				searchFlags) < 0) {
			TRACE("  avformat_seek_file() failed.\n");
			// Try to fall back to av_seek_frame()
			timeStamp = _ConvertToStreamTimeBase(timeStamp);
			if (av_seek_frame(fContext, fStream->index, timeStamp,
				searchFlags) < 0) {
				TRACE("  avformat_seek_frame() failed as well.\n");
				// Fall back to seeking to the beginning by bytes
				timeStamp = 0;
				if (av_seek_frame(fContext, fStream->index, timeStamp,
						AVSEEK_FLAG_BYTE) < 0) {
					TRACE("  avformat_seek_frame() by bytes failed as "
						"well.\n");
					// Do not propagate error in any case. We fail if we can't
					// read another packet.
				} else
					*time = 0;
			}
		}

		// Our last packet is toast in any case. Read the next one so
		// we know where we really sought.
		bigtime_t foundTime = *time;

		fReusePacket = false;
		if (_NextPacket(true) == B_OK) {
			if (fPacket.pts != kNoPTSValue)
				foundTime = _ConvertFromStreamTimeBase(fPacket.pts);
			else
				TRACE_SEEK("  no PTS in packet after seeking\n");
		} else
			TRACE_SEEK("  _NextPacket() failed!\n");

		*time = foundTime;
		TRACE_SEEK("  sought time: %.2fs\n", *time / 1000000.0);
		*frame = (uint64)(*time * frameRate / 1000000.0 + 0.5);
		TRACE_SEEK("  sought frame: %lld\n", *frame);
	}

	return B_OK;
}


status_t
StreamBase::GetNextChunk(const void** chunkBuffer,
	size_t* chunkSize, media_header* mediaHeader)
{
	BAutolock _(fStreamLock);

	TRACE_PACKET("StreamBase::GetNextChunk()\n");

	// Get the last stream DTS before reading the next packet, since
	// then it points to that one.
	int64 lastStreamDTS = fStream->cur_dts;

	status_t ret = _NextPacket(false);
	if (ret != B_OK) {
		*chunkBuffer = NULL;
		*chunkSize = 0;
		return ret;
	}

	// NOTE: AVPacket has a field called "convergence_duration", for which
	// the documentation is quite interesting. It sounds like it could be
	// used to know the time until the next I-Frame in streams that don't
	// let you know the position of keyframes in another way (like through
	// the index).

	// According to libavformat documentation, fPacket is valid until the
	// next call to av_read_frame(). This is what we want and we can share
	// the memory with the least overhead.
	*chunkBuffer = fPacket.data;
	*chunkSize = fPacket.size;

	if (mediaHeader != NULL) {
		mediaHeader->type = fFormat.type;
		mediaHeader->buffer = 0;
		mediaHeader->destination = -1;
		mediaHeader->time_source = -1;
		mediaHeader->size_used = fPacket.size;
		if (fPacket.pts != kNoPTSValue) {
//TRACE("  PTS: %lld (time_base.num: %d, .den: %d), stream DTS: %lld\n",
//fPacket.pts, fStream->time_base.num, fStream->time_base.den,
//fStream->cur_dts);
			mediaHeader->start_time = _ConvertFromStreamTimeBase(fPacket.pts);
		} else {
//TRACE("  PTS (stream): %lld (time_base.num: %d, .den: %d), stream DTS: %lld\n",
//lastStreamDTS, fStream->time_base.num, fStream->time_base.den,
//fStream->cur_dts);
			mediaHeader->start_time
				= _ConvertFromStreamTimeBase(lastStreamDTS);
		}
		mediaHeader->file_pos = fPacket.pos;
		mediaHeader->data_offset = 0;
		switch (mediaHeader->type) {
			case B_MEDIA_RAW_AUDIO:
				break;
			case B_MEDIA_ENCODED_AUDIO:
				mediaHeader->u.encoded_audio.buffer_flags
					= (fPacket.flags & PKT_FLAG_KEY) ? B_MEDIA_KEY_FRAME : 0;
				break;
			case B_MEDIA_RAW_VIDEO:
				mediaHeader->u.raw_video.line_count
					= fFormat.u.raw_video.display.line_count;
				break;
			case B_MEDIA_ENCODED_VIDEO:
				mediaHeader->u.encoded_video.field_flags
					= (fPacket.flags & PKT_FLAG_KEY) ? B_MEDIA_KEY_FRAME : 0;
				mediaHeader->u.encoded_video.line_count
					= fFormat.u.encoded_video.output.display.line_count;
				break;
			default:
				break;
		}
	}

//	static bigtime_t pts[2];
//	static bigtime_t lastPrintTime = system_time();
//	static BLocker printLock;
//	if (fStream->index < 2) {
//		if (fPacket.pts != kNoPTSValue)
//			pts[fStream->index] = _ConvertFromStreamTimeBase(fPacket.pts);
//		printLock.Lock();
//		bigtime_t now = system_time();
//		if (now - lastPrintTime > 1000000) {
//			printf("PTS: %.4f/%.4f, diff: %.4f\r", pts[0] / 1000000.0,
//				pts[1] / 1000000.0, (pts[0] - pts[1]) / 1000000.0);
//			fflush(stdout);
//			lastPrintTime = now;
//		}
//		printLock.Unlock();
//	}

	return B_OK;
}


// #pragma mark -


/*static*/ int
StreamBase::_Read(void* cookie, uint8* buffer, int bufferSize)
{
	StreamBase* stream = reinterpret_cast<StreamBase*>(cookie);

	BAutolock _(stream->fSourceLock);

	TRACE_IO("StreamBase::_Read(%p, %p, %d) position: %lld/%lld\n",
		cookie, buffer, bufferSize, stream->fPosition,
		stream->fSource->Position());

	if (stream->fPosition != stream->fSource->Position()) {
		off_t position
			= stream->fSource->Seek(stream->fPosition, SEEK_SET);
		if (position != stream->fPosition)
			return -1;
	}

	ssize_t read = stream->fSource->Read(buffer, bufferSize);
	if (read > 0)
		stream->fPosition += read;

	TRACE_IO("  read: %ld\n", read);
	return (int)read;

}


/*static*/ off_t
StreamBase::_Seek(void* cookie, off_t offset, int whence)
{
	TRACE_IO("StreamBase::_Seek(%p, %lld, %d)\n",
		cookie, offset, whence);

	StreamBase* stream = reinterpret_cast<StreamBase*>(cookie);

	BAutolock _(stream->fSourceLock);

	// Support for special file size retrieval API without seeking
	// anywhere:
	if (whence == AVSEEK_SIZE) {
		off_t size;
		if (stream->fSource->GetSize(&size) == B_OK)
			return size;
		return -1;
	}

	// If not requested to seek to an absolute position, we need to
	// confirm that the stream is currently at the position that we
	// think it is.
	if (whence != SEEK_SET
		&& stream->fPosition != stream->fSource->Position()) {
		off_t position
			= stream->fSource->Seek(stream->fPosition, SEEK_SET);
		if (position != stream->fPosition)
			return -1;
	}

	off_t position = stream->fSource->Seek(offset, whence);
	TRACE_IO("  position: %lld\n", position);
	if (position < 0)
		return -1;

	stream->fPosition = position;

	return position;
}


status_t
StreamBase::_NextPacket(bool reuse)
{
	TRACE_PACKET("StreamBase::_NextPacket(%d)\n", reuse);

	if (fReusePacket) {
		// The last packet was marked for reuse, so we keep using it.
		TRACE_PACKET("  re-using last packet\n");
		fReusePacket = reuse;
		return B_OK;
	}

	av_free_packet(&fPacket);

	while (true) {
		if (av_read_frame(fContext, &fPacket) < 0) {
			// NOTE: Even though we may get the error for a different stream,
			// av_read_frame() is not going to be successful from here on, so
			// it doesn't matter
			fReusePacket = false;
			return B_LAST_BUFFER_ERROR;
		}

		if (fPacket.stream_index == Index())
			break;

		// This is a packet from another stream, ignore it.
		av_free_packet(&fPacket);
	}

	// Mark this packet with the new reuse flag.
	fReusePacket = reuse;
	return B_OK;
}


int64_t
StreamBase::_ConvertToStreamTimeBase(bigtime_t time) const
{
	int64 timeStamp = int64_t((double)time * fStream->time_base.den
		/ (1000000.0 * fStream->time_base.num) + 0.5);
	if (fStream->start_time != kNoPTSValue)
		timeStamp += fStream->start_time;
	return timeStamp;
}


bigtime_t
StreamBase::_ConvertFromStreamTimeBase(int64_t time) const
{
	if (fStream->start_time != kNoPTSValue)
		time -= fStream->start_time;

	return bigtime_t(1000000.0 * time * fStream->time_base.num
		/ fStream->time_base.den + 0.5);
}


// #pragma mark - AVFormatReader::Stream


class AVFormatReader::Stream : public StreamBase {
public:
								Stream(BPositionIO* source,
									BLocker* streamLock);
	virtual						~Stream();

	// Setup this stream to point to the AVStream at the given streamIndex.
	// This will also initialize the media_format.
	virtual	status_t			Init(int32 streamIndex);

			status_t			GetMetaData(BMessage* data);

	// Support for AVFormatReader
			status_t			GetStreamInfo(int64* frameCount,
									bigtime_t* duration, media_format* format,
									const void** infoBuffer,
									size_t* infoSize) const;

			status_t			FindKeyFrame(uint32 flags, int64* frame,
									bigtime_t* time) const;
	virtual	status_t			Seek(uint32 flags, int64* frame,
									bigtime_t* time);

private:
	mutable	BLocker				fLock;

			struct KeyframeInfo {
				bigtime_t		requestedTime;
				int64			requestedFrame;
				bigtime_t		reportedTime;
				int64			reportedFrame;
				uint32			seekFlags;
			};
	mutable	KeyframeInfo		fLastReportedKeyframe;
	mutable	StreamBase*			fGhostStream;
};



AVFormatReader::Stream::Stream(BPositionIO* source, BLocker* streamLock)
	:
	StreamBase(source, streamLock, &fLock),
	fLock("stream lock"),
	fGhostStream(NULL)
{
	fLastReportedKeyframe.requestedTime = 0;
	fLastReportedKeyframe.requestedFrame = 0;
	fLastReportedKeyframe.reportedTime = 0;
	fLastReportedKeyframe.reportedFrame = 0;
}


AVFormatReader::Stream::~Stream()
{
	delete fGhostStream;
}


status_t
AVFormatReader::Stream::Init(int32 virtualIndex)
{
	TRACE("AVFormatReader::Stream::Init(%ld)\n", virtualIndex);

	status_t ret = StreamBase::Init(virtualIndex);
	if (ret != B_OK)
		return ret;

	// Get a pointer to the AVCodecContext for the stream at streamIndex.
	AVCodecContext* codecContext = fStream->codec;

#if 0
// stippi: Here I was experimenting with the question if some fields of the
// AVCodecContext change (or get filled out at all), if the AVCodec is opened.
	class CodecOpener {
	public:
		CodecOpener(AVCodecContext* context)
		{
			fCodecContext = context;
			AVCodec* codec = avcodec_find_decoder(context->codec_id);
			fCodecOpen = avcodec_open(context, codec) >= 0;
			if (!fCodecOpen)
				TRACE("  failed to open the codec!\n");
		}
		~CodecOpener()
		{
			if (fCodecOpen)
				avcodec_close(fCodecContext);
		}
	private:
		AVCodecContext*		fCodecContext;
		bool				fCodecOpen;
	} codecOpener(codecContext);
#endif

	// initialize the media_format for this stream
	media_format* format = &fFormat;
	memset(format, 0, sizeof(media_format));

	media_format_description description;

	// Set format family and type depending on codec_type of the stream.
	switch (codecContext->codec_type) {
		case AVMEDIA_TYPE_AUDIO:
			if ((codecContext->codec_id >= CODEC_ID_PCM_S16LE)
				&& (codecContext->codec_id <= CODEC_ID_PCM_U8)) {
				TRACE("  raw audio\n");
				format->type = B_MEDIA_RAW_AUDIO;
				description.family = B_ANY_FORMAT_FAMILY;
				// This will then apparently be handled by the (built into
				// BMediaTrack) RawDecoder.
			} else {
				TRACE("  encoded audio\n");
				format->type = B_MEDIA_ENCODED_AUDIO;
				description.family = B_MISC_FORMAT_FAMILY;
				description.u.misc.file_format = 'ffmp';
			}
			break;
		case AVMEDIA_TYPE_VIDEO:
			TRACE("  encoded video\n");
			format->type = B_MEDIA_ENCODED_VIDEO;
			description.family = B_MISC_FORMAT_FAMILY;
			description.u.misc.file_format = 'ffmp';
			break;
		default:
			TRACE("  unknown type\n");
			format->type = B_MEDIA_UNKNOWN_TYPE;
			return B_ERROR;
			break;
	}

	if (format->type == B_MEDIA_RAW_AUDIO) {
		// We cannot describe all raw-audio formats, some are unsupported.
		switch (codecContext->codec_id) {
			case CODEC_ID_PCM_S16LE:
				format->u.raw_audio.format
					= media_raw_audio_format::B_AUDIO_SHORT;
				format->u.raw_audio.byte_order
					= B_MEDIA_LITTLE_ENDIAN;
				break;
			case CODEC_ID_PCM_S16BE:
				format->u.raw_audio.format
					= media_raw_audio_format::B_AUDIO_SHORT;
				format->u.raw_audio.byte_order
					= B_MEDIA_BIG_ENDIAN;
				break;
			case CODEC_ID_PCM_U16LE:
//				format->u.raw_audio.format
//					= media_raw_audio_format::B_AUDIO_USHORT;
//				format->u.raw_audio.byte_order
//					= B_MEDIA_LITTLE_ENDIAN;
				return B_NOT_SUPPORTED;
				break;
			case CODEC_ID_PCM_U16BE:
//				format->u.raw_audio.format
//					= media_raw_audio_format::B_AUDIO_USHORT;
//				format->u.raw_audio.byte_order
//					= B_MEDIA_BIG_ENDIAN;
				return B_NOT_SUPPORTED;
				break;
			case CODEC_ID_PCM_S8:
				format->u.raw_audio.format
					= media_raw_audio_format::B_AUDIO_CHAR;
				break;
			case CODEC_ID_PCM_U8:
				format->u.raw_audio.format
					= media_raw_audio_format::B_AUDIO_UCHAR;
				break;
			default:
				return B_NOT_SUPPORTED;
				break;
		}
	} else {
		if (description.family == B_MISC_FORMAT_FAMILY)
			description.u.misc.codec = codecContext->codec_id;

		BMediaFormats formats;
		status_t status = formats.GetFormatFor(description, format);
		if (status < B_OK)
			TRACE("  formats.GetFormatFor() error: %s\n", strerror(status));

		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32*)format->user_data = codecContext->codec_tag;
		format->user_data[4] = 0;
	}

	format->require_flags = 0;
	format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;

	switch (format->type) {
		case B_MEDIA_RAW_AUDIO:
			format->u.raw_audio.frame_rate = (float)codecContext->sample_rate;
			format->u.raw_audio.channel_count = codecContext->channels;
			format->u.raw_audio.channel_mask = codecContext->channel_layout;
			format->u.raw_audio.byte_order
				= avformat_to_beos_byte_order(codecContext->sample_fmt);
			format->u.raw_audio.format
				= avformat_to_beos_format(codecContext->sample_fmt);
			format->u.raw_audio.buffer_size = 0;

			// Read one packet and mark it for later re-use. (So our first
			// GetNextChunk() call does not read another packet.)
			if (_NextPacket(true) == B_OK) {
				TRACE("  successfully determined audio buffer size: %d\n",
					fPacket.size);
				format->u.raw_audio.buffer_size = fPacket.size;
			}
			break;

		case B_MEDIA_ENCODED_AUDIO:
			format->u.encoded_audio.bit_rate = codecContext->bit_rate;
			format->u.encoded_audio.frame_size = codecContext->frame_size;
			// Fill in some info about possible output format
			format->u.encoded_audio.output
				= media_multi_audio_format::wildcard;
			format->u.encoded_audio.output.frame_rate
				= (float)codecContext->sample_rate;
			// Channel layout bits match in Be API and FFmpeg.
			format->u.encoded_audio.output.channel_count
				= codecContext->channels;
			format->u.encoded_audio.multi_info.channel_mask
				= codecContext->channel_layout;
			format->u.encoded_audio.output.byte_order
				= avformat_to_beos_byte_order(codecContext->sample_fmt);
			format->u.encoded_audio.output.format
				= avformat_to_beos_format(codecContext->sample_fmt);
			if (codecContext->block_align > 0) {
				format->u.encoded_audio.output.buffer_size
					= codecContext->block_align;
			} else {
				format->u.encoded_audio.output.buffer_size
					= codecContext->frame_size * codecContext->channels
						* (format->u.encoded_audio.output.format
							& media_raw_audio_format::B_AUDIO_SIZE_MASK);
			}
			break;

		case B_MEDIA_ENCODED_VIDEO:
// TODO: Specifying any of these seems to throw off the format matching
// later on.
//			format->u.encoded_video.avg_bit_rate = codecContext->bit_rate;
//			format->u.encoded_video.max_bit_rate = codecContext->bit_rate
//				+ codecContext->bit_rate_tolerance;

//			format->u.encoded_video.encoding
//				= media_encoded_video_format::B_ANY;

//			format->u.encoded_video.frame_size = 1;
//			format->u.encoded_video.forward_history = 0;
//			format->u.encoded_video.backward_history = 0;

			format->u.encoded_video.output.field_rate = FrameRate();
			format->u.encoded_video.output.interlace = 1;

			format->u.encoded_video.output.first_active = 0;
			format->u.encoded_video.output.last_active
				= codecContext->height - 1;
				// TODO: Maybe libavformat actually provides that info
				// somewhere...
			format->u.encoded_video.output.orientation
				= B_VIDEO_TOP_LEFT_RIGHT;

			// Calculate the display aspect ratio
			AVRational displayAspectRatio;
		    if (codecContext->sample_aspect_ratio.num != 0) {
				av_reduce(&displayAspectRatio.num, &displayAspectRatio.den,
					codecContext->width
						* codecContext->sample_aspect_ratio.num,
					codecContext->height
						* codecContext->sample_aspect_ratio.den,
					1024 * 1024);
				TRACE("  pixel aspect ratio: %d/%d, "
					"display aspect ratio: %d/%d\n",
					codecContext->sample_aspect_ratio.num,
					codecContext->sample_aspect_ratio.den,
					displayAspectRatio.num, displayAspectRatio.den);
		    } else {
				av_reduce(&displayAspectRatio.num, &displayAspectRatio.den,
					codecContext->width, codecContext->height, 1024 * 1024);
				TRACE("  no display aspect ratio (%d/%d)\n",
					displayAspectRatio.num, displayAspectRatio.den);
		    }
			format->u.encoded_video.output.pixel_width_aspect
				= displayAspectRatio.num;
			format->u.encoded_video.output.pixel_height_aspect
				= displayAspectRatio.den;

			format->u.encoded_video.output.display.format
				= pixfmt_to_colorspace(codecContext->pix_fmt);
			format->u.encoded_video.output.display.line_width
				= codecContext->width;
			format->u.encoded_video.output.display.line_count
				= codecContext->height;
			TRACE("  width/height: %d/%d\n", codecContext->width,
				codecContext->height);
			format->u.encoded_video.output.display.bytes_per_row = 0;
			format->u.encoded_video.output.display.pixel_offset = 0;
			format->u.encoded_video.output.display.line_offset = 0;
			format->u.encoded_video.output.display.flags = 0; // TODO

			break;

		default:
			// This is an unknown format to us.
			break;
	}

	// Add the meta data, if any
	if (codecContext->extradata_size > 0) {
		format->SetMetaData(codecContext->extradata,
			codecContext->extradata_size);
		TRACE("  extradata: %p\n", format->MetaData());
	}

	TRACE("  extradata_size: %d\n", codecContext->extradata_size);
//	TRACE("  intra_matrix: %p\n", codecContext->intra_matrix);
//	TRACE("  inter_matrix: %p\n", codecContext->inter_matrix);
//	TRACE("  get_buffer(): %p\n", codecContext->get_buffer);
//	TRACE("  release_buffer(): %p\n", codecContext->release_buffer);

#ifdef TRACE_AVFORMAT_READER
	char formatString[512];
	if (string_for_format(*format, formatString, sizeof(formatString)))
		TRACE("  format: %s\n", formatString);

	uint32 encoding = format->Encoding();
	TRACE("  encoding '%.4s'\n", (char*)&encoding);
#endif

	return B_OK;
}


status_t
AVFormatReader::Stream::GetMetaData(BMessage* data)
{
	BAutolock _(&fLock);

	avmetadata_to_message(fStream->metadata, data);

	return B_OK;
}


status_t
AVFormatReader::Stream::GetStreamInfo(int64* frameCount,
	bigtime_t* duration, media_format* format, const void** infoBuffer,
	size_t* infoSize) const
{
	BAutolock _(&fLock);

	TRACE("AVFormatReader::Stream::GetStreamInfo(%ld)\n",
		VirtualIndex());

	double frameRate = FrameRate();
	TRACE("  frameRate: %.4f\n", frameRate);

	#ifdef TRACE_AVFORMAT_READER
	if (fStream->start_time != kNoPTSValue) {
		bigtime_t startTime = _ConvertFromStreamTimeBase(fStream->start_time);
		TRACE("  start_time: %lld or %.5fs\n", startTime,
			startTime / 1000000.0);
		// TODO: Handle start time in FindKeyFrame() and Seek()?!
	}
	#endif // TRACE_AVFORMAT_READER

	*duration = Duration();

	TRACE("  duration: %lld or %.5fs\n", *duration, *duration / 1000000.0);

	#if 0
	if (fStream->nb_index_entries > 0) {
		TRACE("  dump of index entries:\n");
		int count = 5;
		int firstEntriesCount = min_c(fStream->nb_index_entries, count);
		int i = 0;
		for (; i < firstEntriesCount; i++) {
			AVIndexEntry& entry = fStream->index_entries[i];
			bigtime_t timeGlobal = entry.timestamp;
			bigtime_t timeNative = _ConvertFromStreamTimeBase(timeGlobal);
			TRACE("    [%d] native: %.5fs global: %.5fs\n", i,
				timeNative / 1000000.0f, timeGlobal / 1000000.0f);
		}
		if (fStream->nb_index_entries - count > i) {
			i = fStream->nb_index_entries - count;
			TRACE("    ...\n");
			for (; i < fStream->nb_index_entries; i++) {
				AVIndexEntry& entry = fStream->index_entries[i];
				bigtime_t timeGlobal = entry.timestamp;
				bigtime_t timeNative = _ConvertFromStreamTimeBase(timeGlobal);
				TRACE("    [%d] native: %.5fs global: %.5fs\n", i,
					timeNative / 1000000.0f, timeGlobal / 1000000.0f);
			}
		}
	}
	#endif

	*frameCount = fStream->nb_frames;
//	if (*frameCount == 0) {
		// Calculate from duration and frame rate
		*frameCount = (int64)(*duration * frameRate / 1000000LL);
		TRACE("  frameCount calculated: %lld, from context: %lld\n",
			*frameCount, fStream->nb_frames);
//	} else
//		TRACE("  frameCount: %lld\n", *frameCount);

	*format = fFormat;

	*infoBuffer = fStream->codec->extradata;
	*infoSize = fStream->codec->extradata_size;

	return B_OK;
}


status_t
AVFormatReader::Stream::FindKeyFrame(uint32 flags, int64* frame,
	bigtime_t* time) const
{
	BAutolock _(&fLock);

	if (fContext == NULL || fStream == NULL)
		return B_NO_INIT;

	TRACE_FIND("AVFormatReader::Stream::FindKeyFrame(%ld,%s%s%s%s, "
		"%lld, %lld)\n", VirtualIndex(),
		(flags & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(flags & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_BACKWARD)
			? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		(flags & B_MEDIA_SEEK_CLOSEST_FORWARD)
			? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		*frame, *time);

	bool inLastRequestedRange = false;
	if ((flags & B_MEDIA_SEEK_TO_FRAME) != 0) {
		if (fLastReportedKeyframe.reportedFrame
			<= fLastReportedKeyframe.requestedFrame) {
			inLastRequestedRange
				= *frame >= fLastReportedKeyframe.reportedFrame
					&& *frame <= fLastReportedKeyframe.requestedFrame;
		} else {
			inLastRequestedRange
				= *frame >= fLastReportedKeyframe.requestedFrame
					&& *frame <= fLastReportedKeyframe.reportedFrame;
		}
	} else if ((flags & B_MEDIA_SEEK_TO_FRAME) == 0) {
		if (fLastReportedKeyframe.reportedTime
			<= fLastReportedKeyframe.requestedTime) {
			inLastRequestedRange
				= *time >= fLastReportedKeyframe.reportedTime
					&& *time <= fLastReportedKeyframe.requestedTime;
		} else {
			inLastRequestedRange
				= *time >= fLastReportedKeyframe.requestedTime
					&& *time <= fLastReportedKeyframe.reportedTime;
		}
	}

	if (inLastRequestedRange) {
		*frame = fLastReportedKeyframe.reportedFrame;
		*time = fLastReportedKeyframe.reportedTime;
		TRACE_FIND("  same as last reported keyframe\n");
		return B_OK;
	}

	double frameRate = FrameRate();
	if ((flags & B_MEDIA_SEEK_TO_FRAME) != 0)
		*time = (bigtime_t)(*frame * 1000000.0 / frameRate + 0.5);

	status_t ret;
	if (fGhostStream == NULL) {
		BAutolock _(fSourceLock);

		fGhostStream = new(std::nothrow) StreamBase(fSource, fSourceLock,
			&fLock);
		if (fGhostStream == NULL) {
			TRACE("  failed to allocate ghost stream\n");
			return B_NO_MEMORY;
		}

		ret = fGhostStream->Open();
		if (ret != B_OK) {
			TRACE("  ghost stream failed to open: %s\n", strerror(ret));
			return B_ERROR;
		}

		ret = fGhostStream->Init(fVirtualIndex);
		if (ret != B_OK) {
			TRACE("  ghost stream failed to init: %s\n", strerror(ret));
			return B_ERROR;
		}
	}
	fLastReportedKeyframe.requestedFrame = *frame;
	fLastReportedKeyframe.requestedTime = *time;
	fLastReportedKeyframe.seekFlags = flags;

	ret = fGhostStream->Seek(flags, frame, time);
	if (ret != B_OK) {
		TRACE("  ghost stream failed to seek: %s\n", strerror(ret));
		return B_ERROR;
	}

	fLastReportedKeyframe.reportedFrame = *frame;
	fLastReportedKeyframe.reportedTime = *time;

	TRACE_FIND("  found time: %.2fs\n", *time / 1000000.0);
	if ((flags & B_MEDIA_SEEK_TO_FRAME) != 0) {
		*frame = int64_t(*time * FrameRate() / 1000000.0 + 0.5);
		TRACE_FIND("  found frame: %lld\n", *frame);
	}

	return B_OK;
}


status_t
AVFormatReader::Stream::Seek(uint32 flags, int64* frame, bigtime_t* time)
{
	BAutolock _(&fLock);

	if (fContext == NULL || fStream == NULL)
		return B_NO_INIT;

	// Put the old requested values into frame/time, since we already know
	// that the sought frame/time will then match the reported values.
	// TODO: Will not work if client changes seek flags (from backwards to
	// forward or vice versa)!!
	bool inLastRequestedRange = false;
	if ((flags & B_MEDIA_SEEK_TO_FRAME) != 0) {
		if (fLastReportedKeyframe.reportedFrame
			<= fLastReportedKeyframe.requestedFrame) {
			inLastRequestedRange
				= *frame >= fLastReportedKeyframe.reportedFrame
					&& *frame <= fLastReportedKeyframe.requestedFrame;
		} else {
			inLastRequestedRange
				= *frame >= fLastReportedKeyframe.requestedFrame
					&& *frame <= fLastReportedKeyframe.reportedFrame;
		}
	} else if ((flags & B_MEDIA_SEEK_TO_FRAME) == 0) {
		if (fLastReportedKeyframe.reportedTime
			<= fLastReportedKeyframe.requestedTime) {
			inLastRequestedRange
				= *time >= fLastReportedKeyframe.reportedTime
					&& *time <= fLastReportedKeyframe.requestedTime;
		} else {
			inLastRequestedRange
				= *time >= fLastReportedKeyframe.requestedTime
					&& *time <= fLastReportedKeyframe.reportedTime;
		}
	}

	if (inLastRequestedRange) {
		*frame = fLastReportedKeyframe.requestedFrame;
		*time = fLastReportedKeyframe.requestedTime;
		flags = fLastReportedKeyframe.seekFlags;
	}

	return StreamBase::Seek(flags, frame, time);
}


// #pragma mark - AVFormatReader


AVFormatReader::AVFormatReader()
	:
	fStreams(NULL),
	fSourceLock("source I/O lock")
{
	TRACE("AVFormatReader::AVFormatReader\n");
}


AVFormatReader::~AVFormatReader()
{
	TRACE("AVFormatReader::~AVFormatReader\n");
	if (fStreams != NULL) {
		// The client was supposed to call FreeCookie() on all
		// allocated streams. Deleting the first stream is always
		// prevented, we delete the other ones just in case.
		int32 count = fStreams[0]->CountStreams();
		for (int32 i = 0; i < count; i++)
			delete fStreams[i];
		delete[] fStreams;
	}
}


// #pragma mark -


const char*
AVFormatReader::Copyright()
{
// TODO: Could not find the equivalent in libavformat >= version 53.
// Use metadata API instead!
//	if (fStreams != NULL && fStreams[0] != NULL)
//		return fStreams[0]->Context()->copyright;
	// TODO: Return copyright of the file instead!
	return "Copyright 2009, Stephan Aßmus";
}


status_t
AVFormatReader::Sniff(int32* _streamCount)
{
	TRACE("AVFormatReader::Sniff\n");

	BPositionIO* source = dynamic_cast<BPositionIO*>(Source());
	if (source == NULL) {
		TRACE("  not a BPositionIO, but we need it to be one.\n");
		return B_NOT_SUPPORTED;
	}

	Stream* stream = new(std::nothrow) Stream(source,
		&fSourceLock);
	if (stream == NULL) {
		ERROR("AVFormatReader::Sniff() - failed to allocate Stream\n");
		return B_NO_MEMORY;
	}

	ObjectDeleter<Stream> streamDeleter(stream);

	status_t ret = stream->Open();
	if (ret != B_OK) {
		TRACE("  failed to detect stream: %s\n", strerror(ret));
		return ret;
	}

	delete[] fStreams;
	fStreams = NULL;

	int32 streamCount = stream->CountStreams();
	if (streamCount == 0) {
		TRACE("  failed to detect any streams: %s\n", strerror(ret));
		return B_ERROR;
	}

	fStreams = new(std::nothrow) Stream*[streamCount];
	if (fStreams == NULL) {
		ERROR("AVFormatReader::Sniff() - failed to allocate streams\n");
		return B_NO_MEMORY;
	}

	memset(fStreams, 0, sizeof(Stream*) * streamCount);
	fStreams[0] = stream;
	streamDeleter.Detach();

	#ifdef TRACE_AVFORMAT_READER
	dump_format(const_cast<AVFormatContext*>(stream->Context()), 0, "", 0);
	#endif

	if (_streamCount != NULL)
		*_streamCount = streamCount;

	return B_OK;
}


void
AVFormatReader::GetFileFormatInfo(media_file_format* mff)
{
	TRACE("AVFormatReader::GetFileFormatInfo\n");

	if (fStreams == NULL)
		return;

	// The first cookie is always there!
	const AVFormatContext* context = fStreams[0]->Context();

	if (context == NULL || context->iformat == NULL) {
		TRACE("  no AVFormatContext or AVInputFormat!\n");
		return;
	}

	const DemuxerFormat* format = demuxer_format_for(context->iformat);

	mff->capabilities = media_file_format::B_READABLE
		| media_file_format::B_KNOWS_ENCODED_VIDEO
		| media_file_format::B_KNOWS_ENCODED_AUDIO
		| media_file_format::B_IMPERFECTLY_SEEKABLE;

	if (format != NULL) {
		// TODO: Check if AVInputFormat has audio only and then use
		// format->audio_family!
		mff->family = format->video_family;
	} else {
		TRACE("  no DemuxerFormat for AVInputFormat!\n");
		mff->family = B_MISC_FORMAT_FAMILY;
	}

	mff->version = 100;

	if (format != NULL) {
		strcpy(mff->mime_type, format->mime_type);
	} else {
		// TODO: Would be nice to be able to provide this from AVInputFormat,
		// maybe by extending the FFmpeg code itself (all demuxers).
		strcpy(mff->mime_type, "");
	}

	if (context->iformat->extensions != NULL)
		strcpy(mff->file_extension, context->iformat->extensions);
	else {
		TRACE("  no file extensions for AVInputFormat.\n");
		strcpy(mff->file_extension, "");
	}

	if (context->iformat->name != NULL)
		strcpy(mff->short_name,  context->iformat->name);
	else {
		TRACE("  no short name for AVInputFormat.\n");
		strcpy(mff->short_name, "");
	}

	if (context->iformat->long_name != NULL)
		sprintf(mff->pretty_name, "%s (FFmpeg)", context->iformat->long_name);
	else {
		if (format != NULL)
			sprintf(mff->pretty_name, "%s (FFmpeg)", format->pretty_name);
		else
			strcpy(mff->pretty_name, "Unknown (FFmpeg)");
	}
}


status_t
AVFormatReader::GetMetaData(BMessage* _data)
{
	// The first cookie is always there!
	const AVFormatContext* context = fStreams[0]->Context();

	if (context == NULL)
		return B_NO_INIT;

	avmetadata_to_message(context->metadata, _data);

	// Add chapter info
	for (unsigned i = 0; i < context->nb_chapters; i++) {
		AVChapter* chapter = context->chapters[i];
		BMessage chapterData;
		chapterData.AddInt64("start", bigtime_t(1000000.0
			* chapter->start * chapter->time_base.num
			/ chapter->time_base.den + 0.5));
		chapterData.AddInt64("end", bigtime_t(1000000.0
			* chapter->end * chapter->time_base.num
			/ chapter->time_base.den + 0.5));

		avmetadata_to_message(chapter->metadata, &chapterData);
		_data->AddMessage("be:chapter", &chapterData);
	}

	// Add program info
	for (unsigned i = 0; i < context->nb_programs; i++) {
		BMessage progamData;
		avmetadata_to_message(context->programs[i]->metadata, &progamData);
		_data->AddMessage("be:program", &progamData);
	}

	return B_OK;
}


// #pragma mark -


status_t
AVFormatReader::AllocateCookie(int32 streamIndex, void** _cookie)
{
	TRACE("AVFormatReader::AllocateCookie(%ld)\n", streamIndex);

	BAutolock _(fSourceLock);

	if (fStreams == NULL)
		return B_NO_INIT;

	if (streamIndex < 0 || streamIndex >= fStreams[0]->CountStreams())
		return B_BAD_INDEX;

	if (_cookie == NULL)
		return B_BAD_VALUE;

	Stream* cookie = fStreams[streamIndex];
	if (cookie == NULL) {
		// Allocate the cookie
		BPositionIO* source = dynamic_cast<BPositionIO*>(Source());
		if (source == NULL) {
			TRACE("  not a BPositionIO, but we need it to be one.\n");
			return B_NOT_SUPPORTED;
		}

		cookie = new(std::nothrow) Stream(source, &fSourceLock);
		if (cookie == NULL) {
			ERROR("AVFormatReader::Sniff() - failed to allocate "
				"Stream\n");
			return B_NO_MEMORY;
		}

		status_t ret = cookie->Open();
		if (ret != B_OK) {
			TRACE("  stream failed to open: %s\n", strerror(ret));
			delete cookie;
			return ret;
		}
	}

	status_t ret = cookie->Init(streamIndex);
	if (ret != B_OK) {
		TRACE("  stream failed to initialize: %s\n", strerror(ret));
		// NOTE: Never delete the first stream!
		if (streamIndex != 0)
			delete cookie;
		return ret;
	}

	fStreams[streamIndex] = cookie;
	*_cookie = cookie;

	return B_OK;
}


status_t
AVFormatReader::FreeCookie(void *_cookie)
{
	BAutolock _(fSourceLock);

	Stream* cookie = reinterpret_cast<Stream*>(_cookie);

	// NOTE: Never delete the first cookie!
	if (cookie != NULL && cookie->VirtualIndex() != 0) {
		if (fStreams != NULL)
			fStreams[cookie->VirtualIndex()] = NULL;
		delete cookie;
	}

	return B_OK;
}


// #pragma mark -


status_t
AVFormatReader::GetStreamInfo(void* _cookie, int64* frameCount,
	bigtime_t* duration, media_format* format, const void** infoBuffer,
	size_t* infoSize)
{
	Stream* cookie = reinterpret_cast<Stream*>(_cookie);
	return cookie->GetStreamInfo(frameCount, duration, format, infoBuffer,
		infoSize);
}


status_t
AVFormatReader::GetStreamMetaData(void* _cookie, BMessage* _data)
{
	Stream* cookie = reinterpret_cast<Stream*>(_cookie);
	return cookie->GetMetaData(_data);
}


status_t
AVFormatReader::Seek(void* _cookie, uint32 seekTo, int64* frame,
	bigtime_t* time)
{
	Stream* cookie = reinterpret_cast<Stream*>(_cookie);
	return cookie->Seek(seekTo, frame, time);
}


status_t
AVFormatReader::FindKeyFrame(void* _cookie, uint32 flags, int64* frame,
	bigtime_t* time)
{
	Stream* cookie = reinterpret_cast<Stream*>(_cookie);
	return cookie->FindKeyFrame(flags, frame, time);
}


status_t
AVFormatReader::GetNextChunk(void* _cookie, const void** chunkBuffer,
	size_t* chunkSize, media_header* mediaHeader)
{
	Stream* cookie = reinterpret_cast<Stream*>(_cookie);
	return cookie->GetNextChunk(chunkBuffer, chunkSize, mediaHeader);
}

