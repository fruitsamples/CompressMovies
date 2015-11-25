/*
	File:		DTSQTUtilities.c

	Contains:	QuickTime functions.

	Written by: 	

	Copyright:	Copyright � 1991-2001 by Apple Computer, Inc., All Rights Reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
                
	Change History (most recent first):
                                    11/7/2001	srk			Carbonized
				7/28/1999	Karl Groethe	Updated for Metrowerks Codewarror Pro 2.1
				

*/


// INCLUDES
#include "DTSQTUtilities.h"


// MOVIE TOOLBOX FUNCTIONS

/*______________________________________________________________________
	QTUIsQuickTimeInstalled - Test if QuickTime is installed.

pascal Boolean    QTUIsQuickTimeInstalled(void) 

DESCRIPTION
	InitQuickTime will test if QuickTime is present. We are not interested in the QuickTime
	version.

ISSUES
	You could combine this function with the QTUGetQTVersion so you could also fetch the
	version level of QT at the same time.
*/

pascal Boolean QTUIsQuickTimeInstalled(void) 
{
	OSErr 	anErr = noErr;
	long 		qtVersion;

	anErr = Gestalt(gestaltQuickTime, &qtVersion); DebugAssert(anErr == noErr);
	if (anErr != noErr)
		return false;		// no QT present
	else
		return true;
}


/*______________________________________________________________________
	QTUIsQuickTimeCFMInstalled - Test if the QuickTime CFM libraries are installed and in the 
	right place.

pascal Boolean    QTUIsQuickTimeCFMInstalled(void) 

DESCRIPTION
	QTUIsQuickTimeCFMInstalled will test if the CFM QuickTime libraries are present (QuickTime 
	PowerPlug, for instance), and if the libraries are still present (this because the libraries are 
	registered once when Gestalt finds then during runtime, and the end user might delete these, 
	or move them to another location later)(.
*/

#if TARGET_CPU_PPC
pascal Boolean QTUIsQuickTimeCFMInstalled(void) 
{
	OSErr 	anErr = noErr;
	long 		qtFeatures = 0L; 

// Test if the library is registered.
	anErr = Gestalt(gestaltQuickTimeFeatures, &qtFeatures); DebugAssert(anErr == noErr);
	
    if (!(  (anErr == noErr)  &&  (qtFeatures & (1 << gestaltPPCQuickTimeLibPresent))  )) // not true
          return false;
          
// Test if a function is available (the library is not moved from the Extension folder),  this is the 
// trick to be used concerning testing if a function is available via CFM.

	if   ( ! CompressImage )
		return false;     
	else 
		return true;
}
#endif // powerc


/*______________________________________________________________________
	QTUGetQTVersion - Return the current QuickTime version number.

pascal long QTUGetQTVersion()

DESCRIPTION
	QTUGetQTVersion is a simple function that will return the current QuickTime version number,
	and if QuickTime is not installed it will return 0L.  The high order word defines the version number, 
	for instance 0x0161 defines version 1.6.1.
	
	You could also directly assign a boolean value stating if a certain version is true by using this
	kind of an expression:
	
	Boolean gHasQT2.0 = (( QTUGetQTVersion() >>  16) & 0xFFFF) >= 0x200;
	
EXAMPLE
	if( (QTUGetQTVersion() >> 16) < 0x150 ) return; // need to work with QT 1.5 or higher.
*/

pascal long QTUGetQTVersion()
{
	long version = 0L;
	
	if(Gestalt(gestaltQuickTime, &version) == noErr)
		return version;
	else
		return 0L;
}


/*______________________________________________________________________
	QTUAreQuickTimeMusicInstrumentsPresent - Test if the Musical Instruments Extension is 
	installed.

pascal Boolean QTUAreQuickTimeMusicInstrumentsPresent(void)

DESCRIPTION
	QTUAreQuickTimeMusicInstrumentsPresent tests if the QuickTime Musical Instruments
	extension (actually a component) is registered. If this is not the case, then most likely
	the extension was never placed into the extension folder, and the end user should be
	informed about this.
*/

pascal Boolean QTUAreQuickTimeMusicInstrumentsPresent(void)
{
	ComponentDescription aCD;
	
	aCD.componentType = 'inst';
	aCD.componentSubType = 'ss  ';
	aCD.componentManufacturer = 'appl';
	
	if(FindNextComponent((Component)0, &aCD) != NULL)
		return true;
	else
		return false;
}


/*______________________________________________________________________
	 QTUPrerollMovie - Preroll the movie before you start the movie.

pascal OSErr QTUPrerollMovie(Movie theMovie)

theMovie				the destination movie for this operation

DESCRIPTION
	QTUPrerollMovie will get the movie time,  duration and preferred rate, and Preroll the movie 
	based on this information. Note that StartMovie already does a PrerollMovie so in that case this 
	is not needed, this is also true of the standard controller that handles the start of movie
	when the keyboard or mouse is used.
*/

pascal OSErr QTUPrerollMovie(Movie theMovie) 
{
	OSErr				anErr = noErr;
	TimeValue 		aTimeValue;
	TimeValue			aMovieDur;
	Fixed 				aPreferredRate;

	aTimeValue 		 = GetMovieTime(theMovie, NULL);
	aMovieDur		 = GetMovieDuration(theMovie);
	aPreferredRate  = GetMoviePreferredRate(theMovie);

	if(aTimeValue == aMovieDur) aTimeValue = 0;

	anErr = PrerollMovie(theMovie, aTimeValue, aPreferredRate); DebugAssert(anErr == noErr);
	
	return anErr;
}


pascal Boolean QTUFileFilter(ParmBlkPtr theParamBlock);


/*______________________________________________________________________
	QTUGetMovie - Get a Movie from a specific file.

pascal Movie QTUGetMovie(FSSpec *theFSSpec, short *theRefNum, short *theResID)

theFSSpec				the specific FSSpec used, if NULL the system will use a standard dialog
							box for the end user to select a file
theRefNum			this is the specific file ref num we want to use later
theResID				this is the specific resource ID we want to use later

DESCRIPTION
	QTUGetMovie will get a movie resource out from a specified file, if the FSSpec is not provided
	then the function will use a StandardGetFilePreview to select the movie.
*/
#if TARGET_OS_WIN32
pascal Movie QTUGetMovie(FSSpec *theFSSpec, short *theRefNum, short *theResID)
{
	OSErr					anErr = noErr;
	SFTypeList			aTypeList = {MovieFileType, 0, 0, 0};
	StandardFileReply	aReply;
	Movie					aMovie = NULL;

// If we are provided with an FSSpec then use it, otherwise do a standardgetfile dialog box and 
// ask the end user to get it.
	if(theFSSpec == NULL || theFSSpec->vRefNum == 0)
	{	
		StandardGetFilePreview( NewFileFilterProc(QTUFileFilter), 1, aTypeList, &aReply);
		if(! aReply.sfGood)
			return NULL;
		
		*theFSSpec = aReply.sfFile;
	}

	// We should have now a usable FSSpec, just double check this once again before continuing.
	DebugAssert(theFSSpec != NULL); if(theFSSpec == NULL) return NULL;
	
	anErr = OpenMovieFile(theFSSpec, theRefNum, fsRdPerm); DebugAssert(anErr == noErr);
	// Note we define fsRdPerm, you could use another flag if needed.

	if(anErr == noErr)
	{
		Str255	aMovieName;
		Boolean	wasChanged;
		
		*theResID = 0;					// want first movie
		
		anErr = NewMovieFromFile(&aMovie, *theRefNum, theResID,
													aMovieName, newMovieActive, &wasChanged);

		DebugAssert(anErr == noErr);

		CloseMovieFile(*theRefNum);
	}
	
	if(anErr != noErr)
		return NULL;
	else
		return aMovie;
}
#endif

/*______________________________________________________________________
	QTUSimpleGetMovie - Get a Movie from a specific file (simpler version)

pascal OSErr QTUSimpleGetMovie(Movie *theMovie)

theMovie				will contain the selected movie when function exits.

DESCRIPTION
	QTUSimpleGetMovie is a simplified version of getting a movie from a file, no need for
	returning refnums, res IDs of keeping track of FSSpecs (compared with QTUGetMovie)
*/
#if TARGET_OS_WIN32
pascal OSErr QTUSimpleGetMovie(Movie *theMovie)
{
	OSErr 					anErr = noErr;
	SFTypeList			aTypeList = {MovieFileType, 0, 0, 0};
	short					resFile = 0;
	short					resID = 0;
	StandardFileReply	aReply;
	Str255					movieName;
	Boolean					wasChanged;

	StandardGetFilePreview(NewFileFilterProc(QTUFileFilter), 1, aTypeList, &aReply);
	if(aReply.sfGood)
	{
		anErr = OpenMovieFile(&aReply.sfFile, &resFile, fsRdPerm); DebugAssert(anErr == noErr);
		if(anErr == noErr)
		{
			anErr = NewMovieFromFile(theMovie, resFile, &resID, movieName, newMovieActive, &wasChanged);
			DebugAssert(anErr == noErr);

			CloseMovieFile(resFile);
		}
	}
	return anErr;
}
#endif

/*______________________________________________________________________
	QTUFileFilter - Skeleton file filter to be used with various MovieToolbox standard dialog utilities.

pascal Boolean QTUFileFilter(ParmBlkPtr theParamBlock)

theParamBlock		specifies a particular ParmBlockPtr

DESCRIPTION
	QTUFileFilter is a skeleton file filter to be used with various MovieToolbox standard dialog utilities
	The function will return a boolean false if it encounters any errors from the Movie toolbox.
*/

pascal Boolean QTUFileFilter(ParmBlkPtr theParamBlock)
{
	#pragma unused(theParamBlock)
	return false;
}


/*______________________________________________________________________
  	QTUSaveMovie - Save and flatten a movie resource into a  file.

pascal OSErr QTUSaveMovie(Movie theMovie)

theMovie			defines the movie to be saved into a file

DESCRIPTION
	QTUSaveMovie will provide a user dialog asking for a file name, and will then save the movie
	into this file. Note that this function will also automatically flatten the movie so that  it's 
	self-contained, and also make it cross-platform (by adding any possible resource forks to
	the end of the data fork. The default name of the movie is also NEWMOVIE.MOV, this reflects
	the way movie file names should be named for cross-platform support (Windows). The default
	creator type is also 'TVOD' so that MoviePlayer will be the default application that opens the
	movie file. If there's an existing movie file with the same name, it will be deleted.
*/

#if TARGET_OS_WIN32
pascal OSErr QTUSaveMovie(Movie theMovie)
{
	OSErr 					anErr = noErr;
	StandardFileReply	anSFReply;
	
	DebugAssert(theMovie != NULL); if(theMovie == NULL) return invalidMovie;
	
	StandardPutFile("\pSave Movie as:" , "\pNEWMOVIE.MOV", &anSFReply); 
	if(anSFReply.sfGood)
	{

		FlattenMovieData(theMovie, flattenAddMovieToDataFork, &anSFReply.sfFile, 
										'TVOD', smSystemScript, createMovieFileDeleteCurFile );
		anErr = GetMoviesError(); DebugAssert(anErr == noErr);
	}
		return anErr;
}
#endif

/*______________________________________________________________________
	QTUFlattenMovieFile - Flatten a movie into a specified file.

pascal OSErr QTUFlattenMovieFile(Movie theMovie, FSSpec *theFile)

theMovie				defines the movie to be flattened
theFile					defines the target file

DESCRIPTION
	FlattenMovie file will take an existing movie, flatten it into a temp file, and then move the
	contents of the temp file into the specified FSSpec. This because there are cases where we 
	can't flatten a movie in place. We will use TickCount as a temp file name.	
	
	Note that we need to dispose the movie inside this function? Why? Well, the file is open as
	long as there's a pointer to it from the movie resource. And we need to delete the original 
	movie file as part of the operation of swapping the files. 
*/

pascal OSErr QTUFlattenMovieFile(Movie theMovie, FSSpec *theFile)
{
	OSErr 		anErr = noErr;
	FSSpec 		tempFile;
	Str255 	tempFileName;
	
	DebugAssert(theMovie != NULL); if(theMovie == NULL) return invalidMovie;
	
	// Create the needed temp file.
	NumToString(TickCount(), tempFileName);
		anErr = FSMakeFSSpec(theFile->vRefNum, theFile->parID, tempFileName, &tempFile);
	if(anErr != fnfErr) return anErr;
	
	// Flatten the movie.
	FlattenMovie(theMovie, flattenAddMovieToDataFork, &tempFile, 'TVOD', smSystemScript, 
							createMovieFileDeleteCurFile, 0, NULL);
	anErr = GetMoviesError();
	if(anErr != noErr)
	{
		FSpDelete(&tempFile);		// remove the temp file
		return anErr;
	}
	
	DisposeMovie(theMovie);
	anErr = FSpDelete(theFile);  ReturnIfError(anErr);
	anErr = FSpRename(&tempFile, theFile->name); ReturnIfError(anErr);
	
	return anErr;
}


// TRACKS AND MEDIA

/*______________________________________________________________________
	QTUMediaTypeInTrack - Check if a particular media type is present in the movie.

pascal Boolean QTUMediaTypeInTrack(Movie theMovie, OSType theMediaType)

theMovie					movie to be tested about the media type
theMediaType			media type we want to test about

DESCRIPTION
	QTUMediaTypeInTrack could be used to scan if a possible media type is present in the movie 
	(video,sound, other media types).
*/

pascal Boolean QTUMediaTypeInTrack(Movie theMovie, OSType theMediaType)
{
	Track 		aTrack = NULL;
	long			aTrackCount = 0;
	long			index;
	OSType		aMediaType;
	Boolean		haveMediaType = false;
	
	aTrackCount = GetMovieTrackCount(theMovie);
	if(aTrackCount == 0)
		return false;				// no tracks in movie
	
	for(index = 1; index <= aTrackCount; index++)
	{
		aTrack = GetMovieIndTrack(theMovie, index);
		GetMediaHandlerDescription( GetTrackMedia(aTrack), &aMediaType, NULL, NULL);
		
		haveMediaType = ( aMediaType == theMediaType);
		if(haveMediaType == true)
			return true;
	}
	return false;			// didn't find the media type track in the movie
}


/*______________________________________________________________________
	QTUGetTrackRect - Get the Rect of a specified  track.

pascal Rect QTUGetTrackRect(Track theTrack)

theTrack				track we are interested in concerning the rect information

DESCRIPTION
	QTUMediaTypeInTrack will take a (visual) track and return the track's Rect boundaries that 
	could be used later for various calculations of the visual track geometries. Note that
	this Rect is meaningful with video tracks (and any other tracks that have geometrical
	dimensions, otherwise this function will return a rect with zero values.
*/

pascal OSErr QTUGetTrackRect(Track theTrack, Rect *theRect)
{
	OSErr	anErr = noErr;
	Fixed	aTrackHeight;
	Fixed	aTrackWidth;

	theRect->top = 0; theRect->left = 0; theRect->bottom = 0; theRect->right = 0;
	
	DebugAssert(theTrack != NULL);
	if(theTrack == NULL)
		return invalidTrack;
		
	GetTrackDimensions(theTrack, &aTrackHeight, &aTrackWidth);
	anErr = GetMoviesError(); DebugAssert(anErr == noErr);
	if(anErr != noErr)
		return anErr;
	
	theRect->right = Fix2Long(aTrackWidth);
	theRect->bottom = Fix2Long(aTrackHeight);
	
	return anErr;
}


/*______________________________________________________________________
	QTUGetVideoMediaPixelDepth - Return the pixel depth of the video media (sample).

pascal short QTUGetVideoMediaPixelDepth(Media theMedia,short index)

theMedia			visual media we want to test concerning pixel depths
index	   			index into the media sample we are interested in

DESCRIPTION
	QTUGetVideoMediaPixelDepth will take a specified video media and an index into the media 
	samples, and look up the pixel depth for the video sample.
*/

pascal short QTUGetVideoMediaPixelDepth(Media theMedia,short index)
{
	OSErr 								anErr = noErr;
	short 								aPixelDepth;
	SampleDescriptionHandle	anImageDesc = NULL;
	OSType				 				mediaType;
	
	DebugAssert(theMedia != NULL);
	DebugAssert(index > 0);
	
	// Test if we are indeed dealing with video media.
	GetMediaHandlerDescription(theMedia, &mediaType, NULL, NULL);
	if(mediaType != VideoMediaType)
		return 0;
		
	anImageDesc = (SampleDescriptionHandle)NewHandle(sizeof(Handle)); DebugAssert(anImageDesc != NULL);
	if(anImageDesc == NULL)
		return 0;
	
	GetMediaSampleDescription(theMedia, index, anImageDesc);
	anErr = GetMoviesError(); DebugAssert(anErr == noErr);
	
	aPixelDepth = (* (ImageDescriptionHandle)anImageDesc)->depth;
	
	DisposeHandle((Handle)anImageDesc);
	
	return aPixelDepth;
}


/*______________________________________________________________________
	QTUCountMediaSamples - Count the amount of known media samples in a movie.

pascal long QTUCountMediaSamples(Movie theMovie, OSType theMediaType)

theMovie					the movie with the track(tracks).	
theMediaType			the type of media we are interested in (video, sound and so on)

DESCRIPTION
	QTUCountMediaSamples will take a specified movie and a media type, and calculate the amount 
	of samples of this particular type. It could be used to find the total amount of video frames in a 
	movie, or sound samples and so on.

	Note that if the movie is long, it will take a long time to go through all the samples, especially 
	in the case of sound samples.

EXAMPLE:
	nFrames = QTUCountMediaSamples(aSourceMovie, VideoMediaType); 

ISSUES
	This function could be modified to count other types of samples by changing the flags definitions 
	(nextTimeSyncSample for key frames and so on).
*/

pascal long QTUCountMediaSamples(Movie theMovie, OSType theMediaType)
{
	long 				numFrames = 0;
	short 			flags = nextTimeMediaSample + nextTimeEdgeOK;
	TimeValue		aDuration = 0;
	TimeValue 	theTime = 0;
	
	GetMovieNextInterestingTime(theMovie, flags, 1, &theMediaType, theTime, 0, &theTime, &aDuration);
	if(theTime == -1) return numFrames;

	flags = nextTimeMediaSample; // Don't include the  nudge after the first interesting time.
	
	while(theTime != -1)  // When the returned time equals -1, then there were no more interesting times.
	{
		numFrames++;
		GetMovieNextInterestingTime(theMovie, flags, 1, &theMediaType, theTime, 0, &theTime, &aDuration);
	}
	
	return numFrames;
}


/*______________________________________________________________________
	QTUGetDurationOfFirstMovieSample - Return the time value of the first sample of a certain 
	media type.

pascal TimeValue  QTUGetDurationOfFirstMovieSample(Movie theMovie, OSType theMediaType)

theMovie						the movie with the media track
theMediaType				specified media type (VideoMediaType, SoundMediaType and so on)

DESCRIPTION
	QTUGetDurationOfFirstMovieSample returns the duration of the first sample of a certain media 
	in the movie. If there is no such sample, the duration is 0.

	This function could be used in known cases where all the samples are assumed to be of the same 
	duration. For instance in such cases the frame count could be calculated as:

	framecount =
			 GetMovieDuration(theMovie)/QTUGetDurationofFirstMovieSample(theMovie, VideoMediaType);

*/

pascal TimeValue  QTUGetDurationOfFirstMovieSample(Movie theMovie, OSType theMediaType)
{
	OSErr 			anErr = noErr;
	TimeValue		interestingDuration = 0;
	short			timeFlags = nextTimeMediaSample+nextTimeEdgeOK;

	GetMovieNextInterestingTime(theMovie, timeFlags, (TimeValue)1, &theMediaType, 0, 
													fixed1, NULL, &interestingDuration);
	anErr = GetMoviesError(); DebugAssert(anErr == noErr);

	return interestingDuration;
}


/*______________________________________________________________________
	QTUCountMaxSoundRate - Calculate the max sound data rate of a possible sound track
   in the movie.

pascal OSErr QTUCountMaxSoundRate(Movie theMovie,long *theMaxSoundRate)

theMovie						the movie with the sound track(tracks).	
theMaxSoundRate			the final returned value

DESCRIPTION
	QTUCountMaxSoundRate (taken from the ConvertToMovieJr file) is a simple function that tries
	to figure out the maximum sound track rate. This is done by looking at all of the sound tracks
	in the source movie, and using the one with the highest sample rate (11khz, 22khz and so on),

	This number could then be used for calculating the maximum data rate by extracting the sound rate and 
	this way we get a loose estimation how much is left for the video data rate.
	
	This is just an approximation, and a better function should take into account non-overlapping
	sound tracks, stereo sound data rates, compressed sound tracks and so on.
*/

pascal OSErr QTUCountMaxSoundRate(Movie theMovie,long *theMaxSoundRate)
{
	OSErr	anErr = noErr;
	short 	index, trackCount;
	
	DebugAssert(theMovie != NULL); if(theMovie == NULL) return invalidMovie;
	*theMaxSoundRate = 0; // just for security we place this value in here
	
	trackCount = GetMovieTrackCount(theMovie);
	
	for(index = 1; index <= trackCount; index++)
	{
		OSType 	aTrackType;
		Track		aTrack = NULL;
		Media		aMedia = NULL;
		
		aTrack = GetMovieIndTrack(theMovie, index);  DebugAssert(aTrack != NULL);
		aMedia = GetTrackMedia(aTrack); DebugAssert(aMedia != NULL);
		anErr = GetMoviesError();  DebugAssert(anErr == noErr);
		if(anErr != noErr) return anErr;
		
		GetMediaHandlerDescription(aMedia, &aTrackType, 0, 0);
		if(aTrackType == SoundMediaType)
		{
			long aRate;
			SampleDescriptionHandle aDesc = NULL;
			
			aDesc = (SampleDescriptionHandle)NewHandle(sizeof(SampleDescription)); DebugAssert(aDesc != NULL);
			GetMediaSampleDescription(aMedia, 1, aDesc);
			anErr = GetMoviesError(); DebugAssert(anErr == noErr);
			if(anErr != noErr)
			{
				DisposeHandle((Handle)aDesc);
				continue;
			}
			
			aRate = (*(SoundDescriptionHandle)aDesc)->sampleRate >> 16;
			if(aRate > *theMaxSoundRate)
				*theMaxSoundRate = aRate;
		}
	}
	return anErr;
}



/*______________________________________________________________________
	QTUGetMovieFrameCount - Return the amount of frames in the movie based on frame rate estimate.

pascal long QTUGetMovieFrameCount(Movie theMovie, long theFrameRate)

theMovie					the movie we want to calculate the frame count for.			
theFrameRate			the expected frame rate of the movie

DESCRIPTION
	QTUGetMovieFrameCount is a simple operation that takes into account the duration of the movie,
	the time scale and a suggested frame rate, and based on this will calculate the 
	amount of frames needed in the movie. We assume that the frame rate will be uniform in the movie.
*/

pascal long QTUGetMovieFrameCount(Movie theMovie, long theFrameRate)
{
	long 		frameCount, duration, timescale;
	float 	exactFrames;
	
	DebugAssert(theMovie != NULL); if(theMovie == NULL) return invalidMovie;

	duration	 	= GetMovieDuration(theMovie);
	timescale 		= GetMovieTimeScale(theMovie);
	exactFrames	= (float)duration * theFrameRate;
	
	frameCount	= exactFrames / timescale / 65536;
	
	if(frameCount == 0)
		frameCount = 1;			// we got to have at least one frame
	
	return frameCount;
}


/*______________________________________________________________________
	QTUCopySoundTracks - Copy any sound track from the source movie to the destination movie.

pascal OSErr QTUCopySoundTracks(Movie theSrcMovie, Movie theDestMovie)

aSourceMovie		 		movie from which to copy the sound tracks			
aDestinationMovie	 		movie to which we will copy the sound tracks.	

DESCRIPTION
	QTUCopySoundTracks will take any sound tracks from the source movie, and copy these over to the
	destination movie. The destination movie might have no sound track, or then these tracks are 
	added to the existing sound tracks.
*/

pascal OSErr QTUCopySoundTracks(Movie theSrcMovie, Movie theDestMovie)
{
	OSErr 	anErr = noErr;
	long		trackCount, index;
	
	DebugAssert(theSrcMovie != NULL); if(theSrcMovie == NULL) return invalidMovie;
	DebugAssert(theDestMovie != NULL); if(theDestMovie == NULL) return invalidMovie;

	trackCount = GetMovieTrackCount(theSrcMovie);
	
	// Loop through each track, look for sound tracks.
	for(index = 1; index <= trackCount; index++)
	{
		OSType aTrackType;
		Track	aSrcTrack, aDestTrack;
		Media	aSrcMedia, aDestMedia;
		
		aSrcTrack = GetMovieIndTrack(theSrcMovie, index);			// get next track and media
		aSrcMedia = GetTrackMedia(aSrcTrack);
		anErr = GetMoviesError(); DebugAssert(anErr == noErr);
		if(anErr != noErr) return anErr;
		
		// try to find sound tracks/media
		GetMediaHandlerDescription(aSrcMedia, &aTrackType, 0, 0);
		if(aTrackType == SoundMediaType)
		{
			// Create the track for the sound media.
			aDestTrack = NewMovieTrack(theDestMovie, 0, 0, GetTrackVolume(aSrcTrack));
			anErr = GetMoviesError(); DebugAssert(anErr == noErr);
			if(anErr != noErr) return anErr;
			
			// Create a media for the sound track and prepare this media for editing.
			aDestMedia = NewTrackMedia(aDestTrack, SoundMediaType, GetMediaTimeScale(aSrcMedia), 0, 0);
			anErr = GetMoviesError(); DebugAssert(anErr == noErr);
			if(anErr != noErr) return anErr;
			
			anErr = BeginMediaEdits(aDestMedia); DebugAssert(anErr == noErr);
			if(anErr != noErr) return anErr;
			
			// Insert the new track into the destination movie starting at time zero and
			// lasting for the entire duration of the movie.
			InsertTrackSegment(aSrcTrack, aDestTrack, 0 , GetTrackDuration(aSrcTrack), 0);
			anErr = GetMoviesError(); DebugAssert(anErr == noErr);
			if(anErr != noErr) return anErr;
			
			// We've done editing the media
			EndMediaEdits(aDestMedia);
		}
	}
	return anErr;
}



/*______________________________________________________________________
	QTUPrintMoviePICT - Print the existing movie frame pict.

pascal Boolean QTUPrintMoviePICTr(Movie theMovie, short x, short y,  long PICTUsed)

theMovie					movie that has the poster		
x,y		    				starting point coordinates where to place the poster on paper

DESCRIPTION
	QTUPrintMoviePICT is a simple function showing how to print movie posters. 

ISSUES
	Note that in a real application we should put the PrStlDialog code into the Print Setup� menu
	function. The reason it's inside this function is that we use this code for quick testing of 
	printing.
*/
#if TARGET_OS_WIN32
pascal OSErr QTUPrintMoviePICT(Movie theMovie, short x, short y, long PICTUsed)
{
	OSErr		anErr = noErr;
	PicHandle 	aPictHandle = NULL;
	THPrint		aTHPrint = NULL;
	GrafPtr	 	aSavedPort;
	TPPrPort	aPrintPort;
	Boolean 	aResult;
	Boolean		isPrinting = false;
	Rect			aPictRect;
	
	DebugAssert(theMovie != NULL); if(theMovie == NULL) return invalidMovie;
	
	GetPort(&aSavedPort);

// Get the PICT to be printed, either the poster pict or the current frame pict.
	switch(PICTUsed)
	{
		case kPrintFrame:
			aPictHandle = GetMoviePict(theMovie, GetMovieTime(theMovie, 0L));
			break;
			

		case kPrintPoster:
			aPictHandle = GetMoviePosterPict(theMovie); 
			break;

		default:
			DebugAssert("Should not happen, incorrect constant used"); goto Closure;
	}

	if(aPictHandle == NULL) goto Closure;


// Get the Print record.
	aTHPrint = (THPrint) NewHandleClear(sizeof(TPrint)); DebugAssert(aTHPrint != NULL);
	if(aTHPrint == NULL) goto Closure;

	PrOpen(); isPrinting = true;
	anErr = PrError(); DebugAssert(anErr == noErr);
		if(anErr != noErr) goto Closure;

	PrintDefault(aTHPrint);

// Move this to Print Setup�if you want to make this look really cool.	
	aResult = PrStlDialog(aTHPrint); DebugAssert(aResult == true);
	if(!aResult) goto Closure;
	
	aResult = PrJobDialog(aTHPrint); DebugAssert(aResult == true);
	if(!aResult) goto Closure;
	
	aPrintPort = PrOpenDoc(aTHPrint, NULL, NULL); DebugAssert(aPrintPort != NULL);
	PrOpenPage(aPrintPort, NULL);
	anErr = PrError(); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto Closure;
	
// Print at x,y position
	aPictRect =  (*aPictHandle)->picFrame;
	OffsetRect(&aPictRect, x - aPictRect.left,  y  - aPictRect.top);
	
	DrawPicture(aPictHandle, &aPictRect);

// If you want to do additional drawing, do it here.
	
	PrClosePage(aPrintPort);
	PrCloseDoc(aPrintPort);
	anErr = PrError(); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto Closure;
	
	if(( *aTHPrint)->prJob.bJDocLoop == bSpoolLoop)
		PrPicFile(aTHPrint, NULL, NULL, NULL, NULL);
	
// Our closure handling.
Closure:
	SetPort(aSavedPort);
	
	if(isPrinting) PrClose();
	if(aPictHandle) KillPicture(aPictHandle);
	if(aTHPrint) DisposeHandle((Handle)aTHPrint);
	return anErr;
}
#endif

/*______________________________________________________________________
	QTUCalculateMovieMemorySize - Calculate how much memory a movie takes in the app heap.

pascal OSErr QTUCalculateMovieMemorySize(Movie theMovie, long *theSize)

theMovie						movie we want to know the size of in the current application heap
theSize							pointer to a long that will contain the movie size in bytes

DESCRIPTION
	QTUCalculateMovieMemorySize will return the amount of bytes it is allocating as a handle
	in the current application heap, if there's not enough space for a temp handle, or if anything
	else fails, the function will return 0L in theSize (and the OSErr).

ISSUES
	Note that possible movie controllers associated with the movie and other constructs will eat up
	memory. What you could do is to do a MacsBug HT before the movie or movies are opened, 
	check the amount of free space, and HT after the movies are opened, figure out the movie sizes 
	using the function below, and calculate the delta from these values.
*/

pascal OSErr QTUCalculateMovieMemorySize(Movie theMovie, long *theSize)
{
	OSErr 	anErr = noErr;
	Handle	tempHandle = NULL;

	DebugAssert(theMovie != NULL); if(theMovie == NULL) return invalidMovie;

	*theSize = 0L;

	tempHandle = NewHandle(sizeof(Movie)); DebugAssert(tempHandle != NULL);
	anErr = MemError(); DebugAssert(anErr == noErr);
	if(anErr != noErr || tempHandle == NULL) goto Closure;

	anErr =  PutMovieIntoHandle(theMovie, tempHandle); DebugAssert(anErr == noErr);

	if(anErr == noErr)	
			*theSize = GetHandleSize(tempHandle);

Closure:
	if(tempHandle != NULL) DisposeHandle(tempHandle);

	return anErr;
}


/*______________________________________________________________________
	QTULoadWholeMovieToRAM - Load the entire active segment (movie) into RAM

pascal OSErr	QTULoadWholeMovieToRAM(Movie theMovie)

theMovie						movie we want to know the size of in the current application heap.

DESCRIPTION
	QTULoadWholeMovieToRAM is an example of how to load movie information into RAM. In
	this case we will load the entire movie, or in other words all the active segments.

ISSUES
	The most likely error returned from this function is due to lack of memory.  You could 
	also fine tune this function by loading partial data based on time or track specifications.

	Loading whole movies is OK if the movies are small, have few tracks with little info (text
	tracks, music tracks and so on), there's a certain performance need  why it makes sense 
	to keep the movie in RAM (looping, other issues), and in general if you know why it's needed.
*/

pascal OSErr	QTULoadWholeMovieToRAM(Movie theMovie)
{
	OSErr anErr = noErr;

	DebugAssert(theMovie != NULL); if(theMovie == NULL) return invalidMovie;

	GoToBeginningOfMovie(theMovie);
	anErr = GetMoviesError(); DebugAssert(anErr == noErr);
	if (anErr != noErr) return anErr;

	anErr = LoadMovieIntoRam(theMovie, GetMovieTime(theMovie, NULL), GetMovieDuration(theMovie), 0);

	return anErr;
}


/*______________________________________________________________________
	QTUPlayMovieSound - Play the movie sound track using the Sound Manager.

pascal OSErr QTUPlayMovieSound(Movie theMovie)

theMovie						movie wherefrom we extract the sound resource

DESCRIPTION
	QTUPlayMovieSound is an example of how to extract the 'snd ' sound resource from the 
	first sound track in a movie, and play this track back using the Sound Manager. This
	sound resource could also be retrieved, or otherwised used in other instances. Note that
	this function is more of an example of how to retrieve sound from a movie; you might
	want to control the start point, duration, and sound track extracted.

*/

pascal OSErr QTUPlayMovieSound(Movie theMovie)
{
	OSErr	anErr = noErr;
	Handle 	tempHandle  = NewHandle(1);
	
	DebugAssert(theMovie != NULL); if(theMovie == NULL) return invalidMovie;

	// Extract first sound track.
	anErr = PutMovieIntoTypedHandle(theMovie, (Track)0, 'snd ', tempHandle, 0, GetMovieDuration(theMovie),
														0, (ComponentInstance)0); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto Closure;
	anErr = MemError(); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto Closure;

	// Play sound resource async.
	anErr = SndPlay(0L, (SndListHandle)tempHandle, true);	 DebugAssert(anErr == noErr);

Closure:
	if(tempHandle) DisposeHandle(tempHandle);

	return anErr;
}


/*______________________________________________________________________
	QTUDrawVideoFrameAtTime - Display a movie video frame at specified movie time.

pascal OSErr QTUDrawVideoFrameAtTime(Movie theMovie, TimeValue atTime)

theMovie						movie we are using
atTime							time value in the movie for the video frame we want to display

DESCRIPTION
	QTUDrawVideoFrameAtTime will display a specific video sample (or frame) at a specified time.
	In other words if we want to draw a frame at time point 600, the nearest video frame
	corresponding to this time value will be shown. 

	We assume that the movie is properly set, and is using a correct portRect or GWorld.

*/

pascal OSErr QTUDrawVideoFrameAtTime(Movie theMovie, TimeValue atTime)
{
	TimeValue totalTime;
	
	OSErr anErr = noErr;
	DebugAssert(theMovie != NULL); if(theMovie == NULL) return paramErr;
	
	totalTime = GetMovieDuration(theMovie);
	if(atTime > totalTime) return paramErr;
	
	if(atTime == 0L) {
		GoToBeginningOfMovie(theMovie); anErr = GetMoviesError(); DebugAssert(anErr == noErr);
		if(anErr) goto Closure;
	}
	else {
		SetMovieTimeValue(theMovie, atTime); anErr = GetMoviesError(); DebugAssert(anErr == noErr);
		if(anErr) goto Closure;
	}
	
	anErr = UpdateMovie(theMovie);  DebugAssert(anErr == noErr);
	if(anErr) goto Closure;
	
	MoviesTask(theMovie, 0L); anErr = GetMoviesError(); DebugAssert(anErr == noErr);

Closure:	
	return anErr;
}



/*______________________________________________________________________
	QTUScrollToNextVideoSample -  Scroll offscreen from one video sample to the next.

pascal OSErr QTUScrollToNextVideoSample(Movie theMovie, TimeValue fromTimePoint, TimeValue toTimePoint)

fromTimePoint						starting time value of the first video sample (frame)
toTimePoint							end time value for the second video sample (frame)

DESCRIPTION
	QTUScrollToNextVideoSample will scroll from one video sample to the other one using offscreen
	GWorlds where the effect is created. 

	We assume that the movie is properly set, and that the movie will use a proper portRect or GWorld.

CREDITS
	Presto Studios for the core idea and some of the code below.

*/
#if TARGET_OS_WIN32
pascal OSErr QTUScrollToNextVideoSample(Movie theMovie, TimeValue fromTimePoint, TimeValue toTimePoint) 
{
	OSErr 				anErr = noErr;
	GWorldPtr			frameGWorld1 = NULL;
	GWorldPtr			frameGWorld2 = NULL;
	PixMapHandle	pixMap1 = NULL;
	PixMapHandle	pixMap2	= NULL;
	CGrafPtr			aSavedPort, moviePort;
	GDHandle			aSavedGDevice, movieGDevice;
	CTabHandle		colorTable;
	short				screenDepth = 0;
	short				screenSize = 0;
	Rect					movieRect, sourceRect, destinationRect;
	RgnHandle			scrollRegion = NULL;
	RgnHandle			clipRegion = NULL;
	short				nSteps;
		
	DebugAssert(theMovie != NULL); if(theMovie == NULL) goto Closure;
	
	//� Store away current portrect and Gdevice, get pixel sizes and color table for GWorld creation purposes.
	GetGWorld(&aSavedPort, &aSavedGDevice);
	
	GetMovieGWorld(theMovie, &moviePort, &movieGDevice);
	screenDepth = (**(**aSavedGDevice).gdPMap).pixelSize;
	colorTable = (**(**aSavedGDevice).gdPMap).pmTable;
	
	//� Adjust the movie box.
	 GetMovieBox(theMovie, &movieRect);  // If you want to offset by 10,10: OffsetRect(&movieRect, 10 -movieRect.left, 10 - movieRect.top);
	 SetMovieBox(theMovie, &movieRect);
	
	//� Create two GWorlds for dual screen writing and possible scrolling transition effects. Lock down pixmaps.
	anErr = NewGWorld(&frameGWorld1, screenDepth, &movieRect, colorTable, NULL, 0); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto Closure;
	anErr = NewGWorld(&frameGWorld2, screenDepth, &movieRect, colorTable, NULL, 0); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto Closure;
	
	pixMap1 = GetGWorldPixMap(frameGWorld1);  if(!LockPixels(pixMap1)) goto Closure;
	pixMap2 = GetGWorldPixMap(frameGWorld2);  if(!LockPixels(pixMap2)) goto Closure;
	
	//� Draw first video sample (frame) to GWorld number 1.
	SetMovieGWorld(theMovie, frameGWorld1, GetGWorldDevice(frameGWorld1));
	SetMovieTimeValue(theMovie, fromTimePoint);
	UpdateMovie(theMovie); MoviesTask(theMovie, 0);
	
	//� Draw second video sample (frame) to GWorld number 2.
	SetMovieGWorld(theMovie, frameGWorld2, GetGWorldDevice(frameGWorld2));
	SetMovieTimeValue(theMovie, toTimePoint); 
	UpdateMovie(theMovie); MoviesTask(theMovie, 0);
	
	//� Create scroll region and store away the current clip region.
	scrollRegion = NewRgn(); DebugAssert(scrollRegion != NULL);
	if(scrollRegion == NULL) goto Closure;
	
	clipRegion = NewRgn(); DebugAssert(clipRegion != NULL);
	if(clipRegion == NULL) goto Closure;
	GetClip(clipRegion); ClipRect(&movieRect);
	
	//� Create the scroll effect.
	screenSize = movieRect.right - movieRect.left;
	
	for(nSteps = 10; nSteps <= screenSize; nSteps += 10)  {
		SetGWorld( frameGWorld1, NULL);
		
		ScrollRect(&movieRect, -10, 0, scrollRegion);
		SetRect(&sourceRect, movieRect.left, movieRect.top, 
				movieRect.left + nSteps, movieRect.bottom);
		SetRect(&destinationRect, movieRect.right - nSteps,
				movieRect.top, movieRect.right, movieRect.bottom);
		
		CopyBits( (BitMap *) *pixMap2, (BitMap *) *pixMap1, &sourceRect, &destinationRect,
					srcCopy, NULL );				// blit from frameGWorld2 to frameGWorld1
		DebugAssert(QDError() == noErr);
		
		SetGWorld(aSavedPort, aSavedGDevice);
		CopyBits( (BitMap *) *pixMap1, (BitMap *) &aSavedPort->portPixMap, &movieRect,
					&movieRect, srcCopy, NULL );   	// blit from frameGWorld1 to screen pixmap
		DebugAssert(QDError() == noErr);
	}

	//� Unlock pixels, restore the original clip region.	
	UnlockPixels(pixMap1); UnlockPixels(pixMap2);
	SetClip(clipRegion);
	
	//� Closure. Clean up if we have handles.
Closure:	
	if(frameGWorld1 != NULL) 		DisposeGWorld(frameGWorld1);
	if(frameGWorld2 != NULL) 		DisposeGWorld(frameGWorld2);
	if(scrollRegion != NULL) 			DisposeRgn(scrollRegion);
	if(clipRegion != NULL) 				DisposeRgn(clipRegion);

	SetMovieGWorld(theMovie, moviePort, movieGDevice);
	SetGWorld(aSavedPort, aSavedGDevice);	

	return anErr;
}
#endif

/*______________________________________________________________________
	QTUGetStartPointOfFirstVideoSample -  Get time value of first sample in the movie.

TimeValue QTUGetStartPointOfFirstVideoSample(Movie theMovie) 

theMovie					movie we are interested in
startPoint					will contain the value of the start point, if the function fails it will contain
								-1.

DESCRIPTION
	QTUGetStartPointOfFirstVideoSample will return the time value of the first video sample found in the
	movie in the startPoint parameter. If the function fails, startPoint will contain -1 and the OSErr is 
	also returned.
*/

pascal OSErr QTUGetStartPointOfFirstVideoSample(Movie theMovie, TimeValue *startPoint) 
{
	OSErr	anErr = noErr;
	OSType	media = VideoMediaType;
	
	*startPoint = -1;

	GetMovieNextInterestingTime(theMovie, nextTimeMediaSample+nextTimeEdgeOK, (TimeValue)1, &media, 0, 
													fixed1, startPoint, NULL);
	anErr = GetMoviesError(); DebugAssert(anErr == noErr);

	return anErr;
}


// IMAGE COMPRESSION MANAGER

/*______________________________________________________________________
	QTUHasCodecLossLessQuality - Test if a specific codec has a lossless mode in a specific bit depth.

Boolean QTUHasCodecLossLessQuality(CodecType theCodec, short thePixelDepth)

theCodec						specifies the Codec Type ('jpeg, 'rle ' and so on).
thePixelDepth				specifies the bit depth (8, 24, 30 and so on). See NIM:QuickTime, page 
									3-70 for more details.

DESCRIPTION
	QTUHasCodecLossLessQuality will test if a specific codec has a lossless spatial compression 
	quality at a certain bit depth. Note that we are not testing the temporal compression qualities.

EXAMPLE OF USE:
	if(QTUHasCodecLossLessQuality('jpeg', 32))  
		printf("JPEG has lossless spatial compression\n");
	else
		printf("JPEG has NOT lossless spatial compression\n");

*/

Boolean QTUHasCodecLossLessQuality(CodecType theCodec, short thePixelDepth)
{
	OSErr 	anErr = noErr;
	CodecQ	aSpatialQuality = codecLosslessQuality;

	anErr = GetCompressionTime(NULL, NULL, thePixelDepth, theCodec, anyCodec, &aSpatialQuality,
						NULL, NULL); DebugAssert(anErr == noErr);
	
	if(aSpatialQuality == codecLosslessQuality)	// still the same?
		return true;
	else
		return false;
}



// MOVIE CONTROLLER FUNCTIONS


/*______________________________________________________________________
	QTUPlayMovieWithMC - Play a specific movie when using movie controllers.

pascal OSErr QTUPlayMovieWithMC( MovieController mc)

mc						specified movie controller to be used

DESCRIPTION
	Playmovie will start a movie using a moviecontroller and a specified movie. Note that it also  
	does a preroll of the movie for performance reasons. 

*/

pascal OSErr QTUPlayMovieWithMC(MovieController mc)
{
// Play normal speed forward, taking into account the possibility 
// of a movie with a nonstandard PreferredRate.
	OSErr	anErr = noErr;
	Fixed 	aRate;
	Movie	aMovie;

	aMovie = MCGetMovie(mc);

	aRate= GetMoviePreferredRate(aMovie);
	anErr = QTUPrerollMovie(aMovie);  // Important: Preroll the movie here.
	DebugAssert(anErr == noErr);

	if(anErr == noErr)
	{
		MCDoAction(mc, mcActionPlay, (void *)aRate);  // note last value
	}
	
	return anErr;
}


/*______________________________________________________________________
	QTUDoIgnoreMCDrags - Disable Drag and Drop facilities of the movie controller environment.

pascal OSErr  QTUDoIgnoreMCDrags(MovieController  mc)

mc						is the specified moviecontroller to be used

DESCRIPTION
	QTUDoIgnoreMCDrags will ensure that the Drag and Drop functionality is not handled within 
	the movie specified by the movie controller.

ISSUES
	Note that this is a workaround in QT 2.0 (test for QT 2.0 or higher if you want to use
	the drag-and-drop support in QT), and this function might not be needed in later QT versions.
*/

pascal OSErr QTUDoIgnoreMCDrags(MovieController  mc)
{
   OSErr   		anErr = noErr;
   GWorldPtr  aTempGWorld;
   Rect 			aTempRect = {0, 0, 20, 20};
   CGrafPtr 	aPort;

   // First create a 1-bit small 20x20 offscreen.
   anErr = NewGWorld( &aTempGWorld, 1, &aTempRect, NULL, NULL, 0L );
   DebugAssert(anErr == noErr);

   if (anErr != noErr)
   {
   		aPort = MCGetControllerPort(mc);									// get the current port
   		MCSetControllerPort(mc, (CGrafPtr)aTempGWorld );		// set mc port to new offscreen
   		MCDoAction(mc, mcActionSetDragEnabled, (void *)false); // don't want dragging
  		MCSetControllerPort(mc, aPort);										// restore mc port
   		DisposeGWorld(aTempGWorld);											// dispose offscreen
   }
   return anErr;
}


/*______________________________________________________________________
	QTUPointInMC - Test if a point is placed in the movie controller rect area or not.

pascal Boolean QTUPointInMC(MovieController mc, WindowRef theWindow, Point where)

mc							is the specified moviecontroller to be used
theWindow				window used for testing for the hit point
where						hit point

DESCRIPTION
	QTUPointInMC is a simple test to check where the mouse was clicked inside the window
	with a movie controller, returns true of the mouse click was inside the movie controller
	rect. See Peter Hoddie's article in develop# 18 for more details (code is from him as well).
*/

pascal Boolean QTUPointInMC(MovieController mc, WindowRef theWindow, Point where)
{
	RgnHandle		aRegion;
	Boolean			result = false;
	
	aRegion = MCGetWindowRgn(mc, theWindow);
   	DebugAssert(aRegion != NULL);
	
	if(aRegion != NULL)
	{
		result = PtInRgn(where, aRegion);
		DisposeRgn(aRegion);
	}
	
	return result;
}


/*______________________________________________________________________
	QTUSelectAllMovie - Select the whole movie time duration with the controller.

pascal OSErr QTUSelectAllMovie(MovieController mc)

mc						is the specified moviecontroller to be used

DESCRIPTION
	QTUSelectAllMovie is an example how to select the whole movie duration using the movie
	controller, this function could be used for Select All menu entries and similar cases.
*/

pascal OSErr QTUSelectAllMovie(MovieController mc)
{
	OSErr 			anErr = noErr;
	TimeRecord  aTimeRecord;
	Movie 			aMovie = NULL;
	
	DebugAssert(mc != NULL);
	if(mc == NULL) return paramErr;
	
	aMovie = MCGetMovie(mc); DebugAssert(aMovie != NULL);
	if(aMovie == NULL) return paramErr;
	
	aTimeRecord.value.hi = 0;
	aTimeRecord.value.lo = 0;
	aTimeRecord.base = 0;
	
	aTimeRecord.scale = GetMovieTimeScale(aMovie);
	anErr = GetMoviesError(); DebugAssert(anErr == noErr);
	if(anErr != noErr) return anErr;
	
	anErr = MCDoAction(mc, mcActionSetSelectionBegin, &aTimeRecord);
	DebugAssert(anErr == noErr);
	if(anErr != noErr) return anErr;
	
	aTimeRecord.value.lo = GetMovieDuration(aMovie);
	anErr = GetMoviesError(); DebugAssert(anErr == noErr);
	if(anErr != noErr) return anErr;
	
	anErr = MCDoAction(mc, mcActionSetSelectionDuration, &aTimeRecord);
	DebugAssert(anErr == noErr);
	
	return anErr;
}


/*______________________________________________________________________
	 QTUResizeMCActionFilter - Example of a movie controller filter that will resize the window 
	where the movie is placed when the controllers themself change. 

pascal Boolean QTUResizeMCActionFilter(MovieController mc, short action, void *params, long refCon)

mc							specified moviecontroller to be used.
action						the action for the mc action filter
params						parameters passed with the action
refCon  					additional long word that could be used for all kinds of purposes

DESCRIPTION
	QTUResizeMCActionFilter is an example of how to create a nice movie controller filter that 
	will handle resizing of the window with the movie, and this will happen every time the controllers
	themselves change. It's also an example of how to write other kinds of movie controller filters.
*/

pascal Boolean QTUResizeMCActionFilter(MovieController mc, short action, void* params , long refCon)
{
	#pragma unused (params)
	Rect aMovieBounds;
	
	switch(action)
	{
		case mcActionControllerSizeChanged:
			MCGetControllerBoundsRect(mc, &aMovieBounds);
			SizeWindow((WindowPtr) refCon, aMovieBounds.right - aMovieBounds.left,
								aMovieBounds.bottom - aMovieBounds.top, true);
			break;
	}
		return false;
}


/*______________________________________________________________________
	QTUResizeMCWindow - Resize a window to either normal size, double size or  half of the movie rect size.

pascal Boolean QTUResizeMCWindow(MovieController mc, WindowPtr theWindow, long theMovieSize, Rect originalSize)

mc								specified moviecontroller to be used
theWindow					window that will be resized
theMovieSize				constant that defines what default size we are interested in,  kNormalSize, kHalfSize, kDoubleSize
originalSize					the original size of the movie, we need to keep track of this one in order to handle the 
 									ambient new sizes (half, double, normal).

DESCRIPTION
	QTUResizeMCWindow is an example of a function how to resize the movie window with the controllers.
	The most common cases is half size, normal size or double size. But nothing hinders to add more sizes
	into this function. Note that if the movie window is doubled, we will get pixel-doubling by the QuickTime
	engine.
*/

pascal OSErr QTUResizeMCWindow(MovieController mc, WindowPtr theWindow, long theMovieSize, Rect originalSize)
{
	OSErr 		anErr = noErr;
	Rect 			aMovieBounds;
	GrafPtr 	aSavedPort;
	
	DebugAssert(mc != NULL); if(mc == NULL) return paramErr;
	DebugAssert(theWindow != NULL); if(theWindow == NULL) return paramErr;
	
	GetPort(&aSavedPort);
	SetPort((GrafPtr)theWindow);
	
	aMovieBounds.top = 0; aMovieBounds.left = 0;

	switch(theMovieSize)
	{
		case kNormalMovieSize:
				MCSetControllerBoundsRect(mc, &originalSize);
				SizeWindow(theWindow, originalSize.right, originalSize.bottom, true);
			break;
		
		case kHalfMovieSize:
				aMovieBounds.right = (originalSize.right - originalSize.left) / 2;
				aMovieBounds.bottom = (originalSize.bottom - originalSize.top) / 2;
				MCSetControllerBoundsRect(mc, &aMovieBounds);
				SizeWindow(theWindow, aMovieBounds.right, aMovieBounds.bottom, true);
			break;
		
		case kDoubleMovieSize:
				aMovieBounds.right = (originalSize.right - originalSize.left) * 2;
				aMovieBounds.bottom = (originalSize.bottom - originalSize.top) * 2;
				MCSetControllerBoundsRect(mc, &aMovieBounds);
				SizeWindow(theWindow, aMovieBounds.right, aMovieBounds.bottom, true);
			break;
		
		default:
			SetPort(aSavedPort);
			anErr = paramErr;
	}
	
	SetPort(aSavedPort);
	return anErr;
}


/*______________________________________________________________________
	QTUResizeMCWindow -Change the movie rate using the movie controller. 

pascal OSErr QTUMCSetMovieRate(MovieController mc, long theRate)

mc							specified moviecontroller to be used
theRate						new rate value, we are using specific constants, see the eQTUMovieRates enum
								in the DTSQTUtilities.h file concerning the values.

DESCRIPTION
	QTUMCSetMovieRate will use an existing movie controller and change the rate. This is a very 
	simple function, but we do have a list of constants that shows the various values that could be used 
	(eQTUMovieRates, DTSQTUtilities.h), and also it shows that if the rate changes from 0 to something 
	else, then we need to preroll the movie. The Apple MM Tuner will make sure the movie is prerolled,
	but we can't assume that every Mac has this extension installed, that's why it's still very important
	to preroll.
	
ISSUES
	Note that movies have stored preferred rates, so if you want to compensate for this factor you need
	to read in this value as well before setting a double or half speed value.
*/

pascal OSErr QTUMCSetMovieRate(MovieController mc, long theRate)
{
	OSErr 	anErr = noErr;
	Fixed 	aRate;
	
	DebugAssert(mc != NULL);  
	if(mc == NULL)
	{
		anErr = paramErr; goto Closure;
	}
	
	// Test if the playrate changes from 0 to a non-zero value, if so then preroll the movie.
	MCDoAction(mc, mcActionGetPlayRate, &aRate);
	if( (aRate == 0) && (theRate != 0) )
	{
		anErr = QTUPrerollMovie(MCGetMovie(mc));			// we are using the DTSQTUtilities function
		DebugAssert(anErr == noErr);
		if(anErr != noErr) return anErr;
	}
		
	anErr = MCDoAction(mc, mcActionPlay, (Ptr) theRate); DebugAssert(anErr == noErr);

Closure:	
	return anErr;
}

// SEQUENCE GRABBER FUNCTIONS
/*______________________________________________________________________
	QTUCreateSequenceGrabber - Create an instance of a sequence grabber for specified window.

pascal SeqGrabComponent QTUCreateSequenceGrabber(WindowPtr theWindow)

theWindow					window where the sequence grabber will operate

DESCRIPTION
	QTUCreateSequenceGrabber will try to open the default sequence grabber component and 
	make sure this component will work in the GWorld of a specified window. 

	If we don't find a suitable sequence grabber, or if we encounter problems, we will return NULL.
*/


pascal SeqGrabComponent QTUCreateSequenceGrabber(WindowPtr theWindow)
{
	OSErr anErr = noErr;
	SeqGrabComponent s = NULL;

	DebugAssert(theWindow != NULL); if(theWindow == NULL) goto Closure;

	s = OpenDefaultComponent(SeqGrabComponentType, 0);

	if(s) // we got a valid one
	{
		anErr = SGInitialize(s); DebugAssert(anErr == noErr);
		if(anErr != noErr) goto Closure;

		anErr = SGSetGWorld(s, (CGrafPtr)theWindow, NULL); DebugAssert(anErr == noErr);
		if(anErr != noErr) goto Closure;
	}

	return s;
Closure:
	return NULL;
}

/*______________________________________________________________________
	QTUCreateSGGrabChannels - Create SG channels, video and audio.

pascal OSErr QTUCreateSGGrabChannels(SeqGrabComponent s, const Rect *theBounds, long theUsage,
																SGChannel *theVideoChannel, SGChannel *theSoundChannel)

s							current active sequence grabber component instance
theBounds				the size of the video channel sequence grabber area
theUsage				any additional flags for the video SG
theVideoChannel	pointer to the video channel we will receive
theSoundChannel	pointer to the audio channel we will receive


DESCRIPTION
	QTUCreateSGGrabChannels will create video and audio SG channels (SGChannels) using the specified
	default SG component.

ISSUES
	We will terminate whenever we can't properly create  a channel (sound, audio), if you still want to
	retrieve a valid channel (let's say the sound one is OK while the we can't open an audio one), you
	could slightly rewrite this code.
*/

pascal OSErr QTUCreateSGGrabChannels(SeqGrabComponent s, const Rect *theBounds, long theUsage,
																SGChannel *theVideoChannel, SGChannel *theSoundChannel)
{
	OSErr 	anErr = noErr;
	long		sgUsage = seqGrabPreview;	// default at least this flag	

	DebugAssert(s != NULL); if(s == NULL) return badSGChannel;

	sgUsage |= theUsage;						// add any other usage flag info now

	// Create Video Channel.
	anErr = SGNewChannel(s, VideoMediaType, theVideoChannel); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto FailureHandling;
	
	anErr = SGSetChannelBounds(*theVideoChannel, theBounds); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto FailureHandling;

	anErr = SGSetChannelUsage(*theVideoChannel, sgUsage); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto FailureHandling;

	// Create Sound Channel.
	anErr = SGNewChannel(s, SoundMediaType, theSoundChannel); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto FailureHandling;

	anErr = SGSetChannelUsage(*theSoundChannel, sgUsage); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto FailureHandling;


	return anErr;

FailureHandling:
	SGDisposeChannel(s, *theVideoChannel); *theVideoChannel = NULL;
	SGDisposeChannel(s, *theSoundChannel); *theSoundChannel = NULL;

	return anErr;
}


/*______________________________________________________________________
	QTUDoesVDIGReceiveVideo - Test if vdig receives a live incoming video signal.

pascal Boolean QTUDoesVDIGReceiveVideo(SeqGrabComponent s)

s					our sequence grabber component instance

DESCRIPTION
	QTUDoesVDIGReceiveVideo test if the currently active vdig is receiving an incoming, live 
	video signal. We assume that all well behaved vdigs set the digiInSignalLock flag.

*/
pascal Boolean QTUDoesVDIGReceiveVideo(SeqGrabComponent s)
{
	OSErr		anErr = noErr;
	long		inputFlags, outFlags;

	DebugAssert(s != NULL); if(s == NULL) goto Closure;

	anErr = VDGetCurrentFlags(s, &inputFlags, &outFlags); DebugAssert(anErr == noErr);
	if(anErr != noErr) goto Closure;

	if(inputFlags & digiInSignalLock)
		return true;

Closure:
	return false;
}


/*______________________________________________________________________
	QTUChangeSGWindowSize - Change window size of the video sequence grabber window.

pascal OSErr QTUChangeSGWindowSize(SeqGrabComponent s, SGChannel videoChannel, 
							WindowPtr theWindow, long width, long height)

s			our sequence grabber component instance
videoChannel	the specified (currently used) video channel
theWindow		window used for the digitizing sequence
width			new width of the digitizer rect
height			new height of the digitizer rect

DESCRIPTION
	QTUChangeSGWindowSize shows how to change the window size for the current digitizing sequence
	taking place in the window. This is more of an example function as there might be other issues
	to be taken into account (such as preference settings and similar issues) while changing the
	bounds of the digitizing rect.

*/

pascal OSErr QTUChangeSGWindowSize(SeqGrabComponent s, SGChannel videoChannel, WindowPtr theWindow, long width, long height)
{
	OSErr		anErr = noErr;
    Rect		portRect;
         
	DebugAssert(theWindow != NULL); if(theWindow == NULL) return paramErr;
	DebugAssert(s != NULL); if(s == NULL) return badSGChannel;
	DebugAssert(videoChannel != NULL); if(videoChannel == NULL) return badSGChannel;

	anErr = SGPause(s, true); DebugAssert(anErr == noErr); if(anErr != noErr) goto Closure;

	SizeWindow(theWindow, width, height, false);
#if TARGET_OS_WIN32
	anErr = SGSetChannelBounds(videoChannel, &theWindow->portRect); DebugAssert(anErr == noErr);
#else
	GetWindowBounds(theWindow, kWindowContentRgn, &portRect);
	anErr = SGSetChannelBounds(videoChannel, &portRect); DebugAssert(anErr == noErr);
#endif
	if(anErr != noErr) goto Closure;

	anErr = SGPause(s, false); DebugAssert(anErr == noErr); 

Closure:
	return anErr;
}



// COMPONENT FUNCTIONS

/*______________________________________________________________________
	QTUDoGetComponent - Get a specific component based on component type and component sub-type.

pascal Component QTUDoGetComponent(OSType theComponentType, OSType theSpecificComponent)

theComponentType					the component type we are interested in
theSpecificComponent				the specific component sub-type we are interested int

DESCRIPTION
	QTUDoGetComponent will get a specific component based on the component type and sub-type.  We have 
	special code for particular components (for instance movieImporttype and movieExporttype), so
	if we specify such types, the function will narrow down the search further for the right components.
	
	The specificComponent is just the special component we want to search for, if the component type is 
	NULL, then the Specific component is the one and only we are interested in. Note that we don't care 
	about the manufacturer information in this function. 
	
	If we don't find a suitable component we will return NULL.
*/

pascal Component QTUDoGetComponent(OSType theComponentType, OSType theSpecificComponent)
{
	ComponentDescription 	aCD;
	Component 					aComponent = NULL;
	
	aCD.componentType = theComponentType;
	aCD.componentSubType = theSpecificComponent;
	aCD.componentManufacturer = 0;
	
	// The following code is inserted for special handling of some known cases.
	if(theComponentType == MovieImportType)
	{
		aCD.componentFlags = canMovieImportFiles;
		aCD.componentFlagsMask = canMovieImportFiles;
	}
	else if(theComponentType == MovieExportType)
	{
		aCD.componentFlags = canMovieExportFiles;
		aCD.componentFlagsMask = canMovieExportFiles;
	}

	// OK, get the component.
	aComponent = FindNextComponent((Component)0, &aCD);
	
	return aComponent;
}


/*______________________________________________________________________
	QTUHasComponentType -Query for a specific component based on component type and 
	component sub-type.

pascal Boolean QTUHasComponentType(OSType theComponentType, OSType theSpecificComponent)

theComponentType					the component type we are interested in
theSpecificComponent				the specific component sub-type we are interested int

DESCRIPTION
	QTUDoGetComponent will query for a specific component based on the component type and sub-type.  
	We have special code for particular components (for instance movieImporttype and movieExporttype),
    so if we query for such types, the function will narrow down the search further for the right 
	components.
	
	The specificComponent is just the special component we want to search for, if the component 
	type is NULL, then the Specific component is the one and only we are interested in. Note that we 
	don't care about the manufacturer information in this function. 
	
	If we don't find a suitable component we will return false, otherwise we will return true.
*/

pascal Boolean QTUHasComponentType(OSType theComponentType, OSType theSpecificComponent)
{
	ComponentDescription aCD;
	
	aCD.componentType = theComponentType;
	aCD.componentSubType = theSpecificComponent;
	aCD.componentManufacturer = 0;
	
	if(theComponentType == MovieImportType)
	{
		aCD.componentFlags = canMovieImportFiles;
		aCD.componentFlagsMask = canMovieImportFiles;
	}
	else if(theComponentType == MovieExportType)
	{
		aCD.componentFlags = canMovieExportFiles;
		aCD.componentFlagsMask = canMovieExportFiles;
	}

	if(FindNextComponent((Component)0, &aCD) != NULL)
		return true;
	else
		return false;
}


//______________________________________________________________________
// T H E    E N D