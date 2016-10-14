/*
 * =====================================================================================
 *
 *       Filename:  report.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  08/18/2016 10:06:57 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Eugen Betke (betke@dkrz.de), Julian Kunkel (juliankunkel@googlemail.com)
 *   Organization:  DKRZ (Deutsches Klimarechenzentrum)
 *
 * =====================================================================================
 */

#include "report.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <mpi.h>
#include <netcdf.h>
#include <netcdf_par.h>

//typedef struct benchmark_t {
//	char* processor;
//	char* testfn;
//	char* block;
//	measurement_t* ms;
//	int nranks;
//	int rank;
//	size_t ndims;
//	size_t dgeom[NDIMS];
//	size_t bgeom[NDIMS];
//	size_t cgeom[NDIMS];
//	procs_t procs;
//	io_mode_t io_mode;
//	size_t block_size;
//	duration_t duration;
//	size_t mssize;
//	int par_access;
//	int storage;
//	bool is_unlimited;
//} benchmark_t;

//static MPI_Datatype create_benchmark_type() {
//{
//	 // Set-up the arguments for the type constructor
//	 MPI_Datatype new_type;
//
//	 int count = 3;
//	 int blocklens[] = {1, 1, 1};
//
//	 MPI_Aint indices[3];
//	 indices[0] = (MPI_Aint)offsetof(benchmark_t, processor);
//	 indices[1] = (MPI_Aint)offsetof(benchmark_t, testfn);
//	 indices[2] = (MPI_Aint)offsetof(benchmark_t, block);
//
//	 MPI_Datatype old_types[] = {MPI_CHAR, MPI_CHAR, MPI_CHAR};
//
//	 MPI_Type_struct(count,blocklens,indices,old_types,&new_type);
//	 MPI_Type_commit(&new_type);
//
//	 return new_type;
//}
//
//
//static void benchmark_send(benchmark_t* bm) {
//	MPI_Datatype type = create_benchmark_type();
//}
//
//static void benchmark_receive(benchmark_t* bm) {
//
//}


/**
 * @brief
 *
 * @param
 */
void report_init(report_t* report){
	report->bm = NULL;
}

/**
 * @brief
 *
 * @param report
 * @param bm
 */
void report_setup(report_t* report, benchmark_t* bm) {
	report->bm = bm;
}

/**
 * @brief
 *
 * @param report
 */
void report_destroy(report_t* report) {
	report->bm = NULL;
}



static void table_write_entry(
		table_t* table,
		const size_t nrows, const size_t ncols, const size_t pos,
		const char* name, const char* type, const char* quantity, const char* unit, const char* value) {
	assert(pos < ncols);
	table_t tab = *table;

	size_t nlen = strlen(name);
	tab[pos][0] = (char*)malloc(sizeof(char*) * nlen + 1);
	strcpy(tab[pos][0], name);

	size_t tlen = strlen(type);
	tab[pos][1] = (char*)malloc(sizeof(char*) * tlen + 1);
	strcpy(tab[pos][1], type);

	size_t qlen = strlen(quantity);
	tab[pos][2] = (char*)malloc(sizeof(char*) * qlen + 1);
	strcpy(tab[pos][2], quantity);

	size_t ulen = strlen(unit);
	tab[pos][3] = (char*)malloc(sizeof(char*) * ulen + 1);
	strcpy(tab[pos][3], unit);

	size_t vlen = strlen(value);
	tab[pos][4] = (char*)malloc(sizeof(char*) * vlen + 1);
	strcpy(tab[pos][4], value);
}


static void table_destroy(table_t* table, const size_t nrows, const size_t ncols) {
	table_t tab = *table;
	for (int row = 0; row < NROWS; ++row) {
		for (int col = 0; col < NCOLS; ++col) {
			free(tab[row][col]);
			tab[row][col] = NULL;
		}
	}
}

static void append_string(char** buf, size_t* buf_size, const char* vb) {
	assert(NULL != buf);
	assert(NULL != vb);
	assert(buf_size > 0);

	const size_t vb_len = strlen(vb);
	const size_t buf_len = strlen(*buf);
	size_t new_buf_size = 0;
	char* new_buf;
	while (vb_len + buf_len + 1 > *buf_size) {
		new_buf_size = *buf_size * 2;
		new_buf = (char*) malloc(sizeof(*new_buf) * new_buf_size);
		strcpy(new_buf, *buf);
		free(*buf);
		*buf_size = new_buf_size;
		*buf = new_buf;
	}
	snprintf(*buf + buf_len, *buf_size - buf_len, "%s", vb);
}


/**
 * @brief
 *
 * @param report
 * @param type
 */
void report_print(const report_t* report, const report_type_t type) {
	int nranks;
	int rank;
	MPI_Comm_size(MPI_COMM_WORLD, &nranks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	benchmark_t* bm = report->bm;
	const size_t dsize = bm->dgeom[0] * bm->dgeom[1] * bm->dgeom[2] * bm->dgeom[3] * sizeof(bm->block[0]);
	const size_t bsize = bm->bgeom[0] * bm->bgeom[1] * bm->bgeom[2] * bm->bgeom[3] * sizeof(bm->block[0]);

	char io_mode_str[10];
	switch (bm->io_mode) {
		case IO_MODE_WRITE:
			strcpy(io_mode_str, "write");
			break;
		case IO_MODE_READ:
			strcpy(io_mode_str, "read");
			break;
	}

	char id[200];
	sprintf(id, "%s:%u:%s", report->bm->processor, report->bm->rank, io_mode_str);

	switch (type) {
		case REPORT_HUMAN:
			{
				const int dist1 = 20;
				const int dist2 = 40;
				const int dist3 = 20;
				const int dist4 = 20;
        const double io_perf = (double) dsize / 1024 / 1024 / (bm->duration.io + bm->duration.open + bm->duration.close);
        const double io_pure_perf = (double) dsize / 1024 / 1024 / bm->duration.io;

				printf("%-*s %-*s %*.10f %-*s\n" , dist1 , id , dist2 , "Open time"                             , dist3 , bm->duration.open  , dist4 , "secs");
				printf("%-*s %-*s %*.10f %-*s\n" , dist1 , id , dist2 , "I/O time"                              , dist3 , bm->duration.io    , dist4 , "secs");
				printf("%-*s %-*s %*.10f %-*s\n" , dist1 , id , dist2 , "Close time"                            , dist3 , bm->duration.close , dist4 , "secs");
				printf("%-*s %-*s %*.4f %-*s\n"  , dist1 , id , dist2 , "I/O Performance (w/o open/close)" , dist3 , io_pure_perf       , dist4 , "MiB/s");
				printf("%-*s %-*s %*.4f %-*s\n"  , dist1 , id , dist2 , "I/O Performance"                       , dist3 , io_perf            , dist4 , "MiB/s");
				printf("%-*s %-*s"  , dist1 , id , dist2 , "I/O Times per block");
				for (size_t i = 0; i < bm->mssize; ++i) {
					char buffer[100];
					snprintf(buffer, 100, "%.10f", time_to_double(time_diff(bm->ms[i].stop, bm->ms[i].start))); // This was wrong for large times, always do: to_double(stop - start) !
					printf("%s ", buffer);
				}
				printf("\n");
			break;
		}
		case REPORT_PARSER:
			{
				// Memory allocation and initialization
				table_t table = malloc(NCOLS * sizeof(*table));
				for (int row = 0; row < NROWS; ++row) {
					for (int col = 0; col < NCOLS; ++col) {
						table[col][row] = NULL;
					}
				}


				// Fill table
				char buffer[100];
				size_t pos = 0;
				table_write_entry(&table, NROWS, NCOLS, pos++, "name", "type", "quantity", "unit", "value");
				table_write_entry(&table, NROWS, NCOLS, pos++, "processor", "text", "label", "", bm->processor);
				snprintf(buffer, 100, "%d", bm->rank);
				table_write_entry(&table, NROWS, NCOLS, pos++, "rank", "int", "", "", buffer);
				snprintf(buffer, 100, "%.10f", bm->duration.io);
				table_write_entry(&table, NROWS, NCOLS, pos++, "time_io", "float", "time", "secs", buffer);
				snprintf(buffer, 100, "%.10f", bm->duration.open);
				table_write_entry(&table, NROWS, NCOLS, pos++, "time_open", "float", "time", "secs", buffer);
				snprintf(buffer, 100, "%.10f", bm->duration.close);
				table_write_entry(&table, NROWS, NCOLS, pos++, "time_close", "float", "time", "secs", buffer);
				size_t msbufsize = 20;
				char* msbuf = malloc(sizeof(*msbuf) * msbufsize);
				msbuf[0] = '\0';
				for (size_t i = 0; i < bm->mssize; ++i) {
					snprintf(buffer, 100, "%.10f", time_to_double(time_diff(bm->ms[i].stop, bm->ms[i].start))); // This was wrong for lare times, always do: to_double(stop - start) !
					append_string(&msbuf, &msbufsize, buffer);
				}
				msbuf[strlen(msbuf) - 1] = '\0'; // Delete the last separator

				table_write_entry(&table, NROWS, NCOLS, pos++, "time_per_block", "text", "time", "secs", msbuf);
				table_write_entry(&table, NROWS, NCOLS, pos++, "io_type", "text", "", "", io_mode_str);
				assert(pos == NCOLS);

				// Make Report
				size_t outbuf_size = 100;
				char* outbuf = (char*) malloc(sizeof(char*) * outbuf_size);
				outbuf[0] = '\0';
				// Print report
				for (int row = 0; row < NROWS; ++row) {
					snprintf(buffer, 100, "benchmark:%d:%s ", bm->rank, table[0][row]);
					append_string(&outbuf, &outbuf_size, buffer);
					for (int col = 1; col < NCOLS - 1; ++col) {
						append_string(&outbuf, &outbuf_size, table[col][row]);
						append_string(&outbuf, &outbuf_size, ":");
					}
					append_string(&outbuf, &outbuf_size, table[NCOLS - 1][row]);
					snprintf(buffer, 100, "\n");
					append_string(&outbuf, &outbuf_size, buffer);
				}

				// Send/Receive Report
				size_t* report_size = (size_t*) malloc(nranks * sizeof(*report_size));
				report_size[0] = outbuf_size;

				// NEW
//				benchmark_t** benchmarks = NULL;
//				if (0 == rank){
//					benchmarks= (benchmark_t**)malloc(sizeof(benchmark_t*) * nranks);
//					for (int i = 1; i < nranks; ++i) {
//						benchmarks[i] = (benchmark_t*) malloc(nranks * sizeof(*benchmarks));
//					}
//				}
//
//				benchmarks[0] = report->bm;
//
//				if (0 == rank){
//					for(int i = 1; i < nranks; ++i){
//						benchmark_send(benchmarks[i]);
//					}
//				}
//				else {
//					benchmark_send(benchmarks[0]);
//				}
				// END: NEW


				if (0 == rank){
					for(int i = 1; i < nranks; ++i){
						MPI_Recv(&report_size[i], 1, MPI_UNSIGNED_LONG_LONG, i, 4710, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					}
				}
				else {
					MPI_Send(&report_size[0], 1, MPI_UNSIGNED_LONG_LONG, 0, 4710, MPI_COMM_WORLD);
				}

				char** report = NULL;
				if (0 == rank){
					report = (char**)malloc(sizeof(char*) * nranks);
					for (int i = 1; i < nranks; ++i) {
						report[i] = (char*)malloc(sizeof(char) * report_size[i]);
					}
				}

				if (0 == rank){
					report[0] = outbuf;
					printf("%s\n", report[0]);
					for(int i = 1; i < nranks; i++){
						MPI_Recv(report[i], report_size[i], MPI_CHAR, i, 4712, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						printf("%s\n", report[i]);
					}
				}
				else {
					MPI_Send(outbuf, outbuf_size, MPI_CHAR, 0, 4712, MPI_COMM_WORLD);
				}

				// Clean up
				table_destroy(&table, NROWS, NCOLS);
				free(msbuf);
				free(outbuf);
				free(report_size);
//				if (0 == rank) {
//					for (int i = 1; i < nranks; ++i) {
//						free(benchmarks[i]);
//					}
//					free(benchmarks);
//				}
				if (0 == rank) {
					for (int i = 1; i < nranks; ++i) {
						free(report[i]);
					}
					free(report);
				}
			}
			break;
	}
}
