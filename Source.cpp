#include "gdal_priv.h"

#include <iostream>
#include <vector>
#include <cstdint>

#define PVR_THRESHOLD 0.18f
#define PPR_THRESHOLD 0.01f
#define GLI_THRESHOLD 0.02f

using namespace std;

int nrows, ncols;
double nodata;
double transform[6];
const char* proj;

GDALDataset* pvr_calc(GDALDataset* in_ds, const char* output_filename)
{
	GDALDataset* out_ds;
	GDALDriver* geoTiff;

	geoTiff = GetGDALDriverManager()->GetDriverByName("GTiff");
	out_ds = geoTiff->Create(output_filename, ncols, nrows, 1, GDT_Float32, NULL);
	out_ds->SetGeoTransform(transform);
	out_ds->GetRasterBand(1)->SetNoDataValue(nodata);
	out_ds->SetProjection(proj);

	uint32_t* green_row = (uint32_t*)CPLMalloc(sizeof(uint32_t)*ncols);
	uint32_t* red_row = (uint32_t*)CPLMalloc(sizeof(uint32_t)*ncols);
	float* output_row = (float*)CPLMalloc(sizeof(float)*ncols);

	for (int i = 0; i < nrows; i++)
	{
		in_ds->GetRasterBand(2)->RasterIO(GF_Read, 0, i, ncols, 1, green_row, ncols, 1, GDT_UInt32, 0, 0);
		in_ds->GetRasterBand(1)->RasterIO(GF_Read, 0, i, ncols, 1, red_row, ncols, 1, GDT_UInt32, 0, 0);
		for (int j = 0; j < ncols; j++)
		{
			output_row[j] = ((float)green_row[j] - red_row[j]) / ((float)green_row[j] + red_row[j]);
		}
		out_ds->GetRasterBand(1)->RasterIO(GF_Write, 0, i, ncols, 1, output_row, ncols, 1, GDT_Float32, 0, 0);
	}

	CPLFree(green_row);
	CPLFree(red_row);
	CPLFree(output_row);

	return out_ds;
}

GDALDataset* ppr_calc(GDALDataset* in_ds, const char* output_filename)
{
	GDALDataset* out_ds;
	GDALDriver* geoTiff;

	geoTiff = GetGDALDriverManager()->GetDriverByName("GTiff");
	out_ds = geoTiff->Create(output_filename, ncols, nrows, 1, GDT_Float32, NULL);
	out_ds->SetGeoTransform(transform);
	out_ds->GetRasterBand(1)->SetNoDataValue(nodata);
	out_ds->SetProjection(proj);

	uint32_t* blue_row = (uint32_t*)CPLMalloc(sizeof(uint32_t)*ncols);
	uint32_t* green_row = (uint32_t*)CPLMalloc(sizeof(uint32_t)*ncols);
	float* output_row = (float*)CPLMalloc(sizeof(float)*ncols);

	for (int i = 0; i < nrows; i++)
	{
		in_ds->GetRasterBand(3)->RasterIO(GF_Read, 0, i, ncols, 1, blue_row, ncols, 1, GDT_UInt32, 0, 0);
		in_ds->GetRasterBand(2)->RasterIO(GF_Read, 0, i, ncols, 1, green_row, ncols, 1, GDT_UInt32, 0, 0);
		for (int j = 0; j < ncols; j++)
		{
			output_row[j] = ((float)green_row[j] - blue_row[j]) / ((float)green_row[j] + blue_row[j]);
		}
		out_ds->GetRasterBand(1)->RasterIO(GF_Write, 0, i, ncols, 1, output_row, ncols, 1, GDT_Float32, 0, 0);
	}

	CPLFree(blue_row);
	CPLFree(green_row);
	CPLFree(output_row);

	return out_ds;
}

GDALDataset* gli_calc(GDALDataset* in_ds, const char* output_filename)
{
	GDALDataset* out_ds;
	GDALDriver* geoTiff;

	geoTiff = GetGDALDriverManager()->GetDriverByName("GTiff");
	out_ds = geoTiff->Create(output_filename, ncols, nrows, 1, GDT_Float32, NULL);
	out_ds->SetGeoTransform(transform);
	out_ds->GetRasterBand(1)->SetNoDataValue(nodata);
	out_ds->SetProjection(proj);

	uint32_t* blue_row = (uint32_t*)CPLMalloc(sizeof(uint32_t)*ncols);
	uint32_t* green_row = (uint32_t*)CPLMalloc(sizeof(uint32_t)*ncols);
	uint32_t* red_row = (uint32_t*)CPLMalloc(sizeof(uint32_t)*ncols);
	float* output_row = (float*)CPLMalloc(sizeof(float)*ncols);

	for (int i = 0; i < nrows; i++)
	{
		in_ds->GetRasterBand(1)->RasterIO(GF_Read, 0, i, ncols, 1, red_row, ncols, 1, GDT_UInt32, 0, 0);
		in_ds->GetRasterBand(3)->RasterIO(GF_Read, 0, i, ncols, 1, blue_row, ncols, 1, GDT_UInt32, 0, 0);
		in_ds->GetRasterBand(2)->RasterIO(GF_Read, 0, i, ncols, 1, green_row, ncols, 1, GDT_UInt32, 0, 0);
		for (int j = 0; j < ncols; j++)
		{
			output_row[j] = ((2 * ((float)green_row[j] - red_row[j] - blue_row[j])) / ((2 * (float)green_row[j] - red_row[j] - blue_row[j])));
		}
		out_ds->GetRasterBand(1)->RasterIO(GF_Write, 0, i, ncols, 1, output_row, ncols, 1, GDT_Float32, 0, 0);
	}

	CPLFree(blue_row);
	CPLFree(green_row);
	CPLFree(red_row);
	CPLFree(output_row);

	return out_ds;
}

void getinfo(GDALDataset* in_ds)
{
	nrows = in_ds->GetRasterBand(1)->GetYSize();
	ncols = in_ds->GetRasterBand(1)->GetXSize();
	nodata = in_ds->GetRasterBand(1)->GetNoDataValue();
	in_ds->GetGeoTransform(transform);
	proj = in_ds->GetProjectionRef();
}


GDALDataset* load_image(const char* filepath)
{
	GDALDataset* ds;
	ds = (GDALDataset*)GDALOpen(filepath, GA_ReadOnly);

	// Get Info
	getinfo(ds);
	return ds;
}

GDALDataset* reclassify(GDALDataset* in_ds, const char* output_filename, float threshold)
{
	GDALDataset* out_ds;
	GDALDriver* geoTiff;

	geoTiff = GetGDALDriverManager()->GetDriverByName("GTiff");
	out_ds = geoTiff->Create(output_filename, ncols, nrows, 1, GDT_Byte, NULL);
	out_ds->SetGeoTransform(transform);
	out_ds->GetRasterBand(1)->SetNoDataValue(nodata);
	out_ds->SetProjection(proj);

	float *input_row = (float*)CPLMalloc(sizeof(float)*ncols);
	uint8_t *output_row = (uint8_t*)CPLMalloc(sizeof(uint8_t)*ncols);

	for (int i = 0; i < nrows; i++)
	{
		in_ds->GetRasterBand(1)->RasterIO(GF_Read, 0, i, ncols, 1, input_row, ncols, 1, GDT_Float32, 0, 0);
		for (int j = 0; j < ncols; j++)
		{		
			if (input_row[j] >= threshold) {
				output_row[j] = 1;
			} else {
				output_row[j] = 0;
			}
		}
		out_ds->GetRasterBand(1)->RasterIO(GF_Write, 0, i, ncols, 1, output_row, ncols, 1, GDT_Byte, 0, 0);
	}

	CPLFree(input_row);
	CPLFree(output_row);

	return out_ds;
}

GDALDataset* sum_indices(GDALDataset* pvr_ds, GDALDataset* ppr_ds, GDALDataset* gli_ds, const char* output_filename)
{
	GDALDataset* out_ds;
	GDALDriver* geoTiff;

	geoTiff = GetGDALDriverManager()->GetDriverByName("GTiff");
	out_ds = geoTiff->Create(output_filename, ncols, nrows, 1, GDT_UInt16, NULL);
	out_ds->SetGeoTransform(transform);
	out_ds->GetRasterBand(1)->SetNoDataValue(nodata);
	out_ds->SetProjection(proj);
	
	uint8_t *pvr_row = (uint8_t*)CPLMalloc(sizeof(uint8_t)*ncols);
	uint8_t *ppr_row = (uint8_t*)CPLMalloc(sizeof(uint8_t)*ncols);
	uint8_t *gli_row = (uint8_t*)CPLMalloc(sizeof(uint8_t)*ncols);
	uint16_t *output_row = (uint16_t*)CPLMalloc(sizeof(uint16_t)*ncols);

	for (int i = 0; i < nrows; i++)
	{
		pvr_ds->GetRasterBand(1)->RasterIO(GF_Read, 0, i, ncols, 1, pvr_row, ncols, 1, GDT_Byte, 0, 0);
		ppr_ds->GetRasterBand(1)->RasterIO(GF_Read, 0, i, ncols, 1, ppr_row, ncols, 1, GDT_Byte, 0, 0);
		gli_ds->GetRasterBand(1)->RasterIO(GF_Read, 0, i, ncols, 1, gli_row, ncols, 1, GDT_Byte, 0, 0);
		for (int j = 0; j < ncols; j++)
		{
			output_row[j] = pvr_row[j] + ppr_row[j] + gli_row[j];
		}
		out_ds->GetRasterBand(1)->RasterIO(GF_Write, 0, i, ncols, 1, output_row, ncols, 1, GDT_UInt16, 0, 0);
	}

	CPLFree(pvr_row);
	CPLFree(ppr_row);
	CPLFree(gli_row);
	CPLFree(output_row);

	return out_ds;
}

bool process(const char* in_filename)
{
	GDALAllRegister();
	// Load Image
	GDALDataset* in_ds = load_image(in_filename);

	// Create Indices
	GDALDataset* pvr_out_ds = pvr_calc(in_ds, "pvr.tif");
	GDALDataset* ppr_out_ds = ppr_calc(in_ds, "ppr.tif");
	GDALDataset* gli_out_ds = gli_calc(in_ds, "gli.tif");

	// Reclassify
	GDALDataset* pvr_reclass = reclassify(pvr_out_ds, "pvr_reclass.tif", PVR_THRESHOLD);
	GDALDataset* ppr_reclass = reclassify(ppr_out_ds, "ppr_reclass.tif", PPR_THRESHOLD);
	GDALDataset* gli_reclass = reclassify(gli_out_ds, "gli_reclass.tif", GLI_THRESHOLD);

	// Sum Indices
	GDALDataset* sum = sum_indices(pvr_reclass, ppr_reclass, gli_reclass, "sum_idx.tif");

	return true;
}

int main(int argc, char** argv)
{
	bool run = false;
	run = process(argv[1]);
	if (run == false) cerr << "Error" << endl;

	return 0;
}