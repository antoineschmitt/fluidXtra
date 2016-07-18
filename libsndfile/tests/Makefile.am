noinst_PROGRAMS = read_seek_test write_read_test lossy_comp_test error_test ulaw_test alaw_test

SNDFILEDIR =../src
INCLUDES = -I$(srcdir)/$(SNDFILEDIR)

read_seek_test_SOURCES = read_seek_test.c 
read_seek_test_LDADD = $(SNDFILEDIR)/libsndfile.la

write_read_test_SOURCES = write_read_test.c 
write_read_test_LDADD = $(SNDFILEDIR)/libsndfile.la

lossy_comp_test_SOURCES = lossy_comp_test.c 
lossy_comp_test_LDADD = $(SNDFILEDIR)/libsndfile.la -lm

error_test_SOURCES = error_test.c 
error_test_LDADD = $(SNDFILEDIR)/libsndfile.la

ulaw_test_SOURCES = ulaw_test.c 
ulaw_test_LDADD = $(SNDFILEDIR)/libsndfile.la

alaw_test_SOURCES = alaw_test.c 
alaw_test_LDADD = $(SNDFILEDIR)/libsndfile.la

check: generic-test ulaw_test first-test wav-tests aiff-tests au-tests raw-tests

generic-test : error_test
	./error_test
	./ulaw_test
	./alaw_test
	@echo "--------------------------------------------------------------------"
	@echo "  libsndfile passed generic tests."
	@echo "--------------------------------------------------------------------"

first-test: read_seek_test
	./read_seek_test wav
	./read_seek_test aiff
	./read_seek_test au
	@echo "--------------------------------------------------------------------"
	@echo "  libsndfile passed first set of tests."
	@echo "--------------------------------------------------------------------"

wav-tests: write_read_test
	./write_read_test wav
	./write_read_test wav_float
	./lossy_comp_test wav_ima
	./lossy_comp_test wav_msadpcm
	./lossy_comp_test wav_ulaw
	./lossy_comp_test wav_alaw
	@echo "--------------------------------------------------------------------"
	@echo "  libsndfile passed tests on WAV files."
	@echo "--------------------------------------------------------------------"

aiff-tests: write_read_test
	./write_read_test aiff
	@echo "--------------------------------------------------------------------"
	@echo "  libsndfile passed tests on AIFF files."
	@echo "--------------------------------------------------------------------"

au-tests: write_read_test
	./write_read_test au
	./write_read_test aule
	./lossy_comp_test au_ulaw
	./lossy_comp_test au_alaw
	./lossy_comp_test aule_ulaw
	./lossy_comp_test aule_alaw
	@echo "--------------------------------------------------------------------"
	@echo "  libsndfile passed tests on AU files."
	@echo "--------------------------------------------------------------------"

raw-tests: write_read_test
	./write_read_test raw
	@echo "--------------------------------------------------------------------"
	@echo "  libsndfile passed tests on RAW (header-less PCM) files."
	@echo "--------------------------------------------------------------------"

