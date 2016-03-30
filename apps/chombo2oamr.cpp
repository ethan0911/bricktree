#include <stdio.h>

#define CH_SPACEDIM 3
#define CH_LANG_CC
#define CH_USE_HDF5

#include "CH_assert.H"
#include "CH_HDF5.H"
#include "AMRIO.H"

#include <iostream>
#include <string>

float *allocate(const IntVect &size)
{
    printf("allocate of %ld bytes\n",
           sizeof(float) * size[0] * size[1] * size[2]);
    return new float[size[0] * size[1] * size[2]];
}

inline void setval(float *data, float val,
                   const IntVect &size, const IntVect &loc)
{
    data[loc[2] * size[0] * size [1] + loc[1] * size [0] + loc[0]] = val;
}

inline float getval(float *data, const IntVect &size, const IntVect &loc)
{
    return data[loc[2] * size[0] * size [1] + loc[1] * size [0] + loc[0]];
}


float *resize(float *data, IntVect &size)
{
    IntVect newsize = size * 4;
    float *resampled = allocate(newsize);

    for (int z = 0; z < newsize[2]; z++) {
        for (int y = 0; y < newsize[1]; y++) {
            for (int x = 0; x < newsize[0]; x++) {
                IntVect loc(x, y, z);
                IntVect oldloc = loc / 4;
                setval(resampled, getval(data, size, oldloc), newsize, loc);
            }
        }
    }

    delete [] data;
    size = newsize;
    return resampled;
}

void write(float *data, const IntVect &size)
{
    FILE *raw = fopen("gear.raw", "wb");
    fwrite(data, sizeof(float), size[0]*size[1]*size[2], raw);
    fclose(raw);

    FILE *osp = fopen("gear.osp", "wt");
    fprintf(osp,
            "<?xml version=\"1.0\"?>\n"
            "<volume name=\"volume\">\n"
            "  <dimensions>%d %d %d</dimensions>\n"
            "  <voxelType>float</voxelType>\n"
            "  <samplingRate>1.0</samplingRate>\n"
            "  <filename>gear.raw</filename>\n"
            "</volume>\n",
            size[0], size[1], size[2]);
    fclose(osp);
}

int main(int argc, char **argv)
{
    HDF5Handle handle;
    HDF5HeaderData header;
    handle.open("gear.hdf5", HDF5Handle::OPEN_RDONLY);

    header.readFromFile(handle);

    int levels = header.m_int["num_levels"];
    int numComponents = header.m_int["num_components"];

    printf("levels: %d\n", levels);
    printf("components: %d\n", numComponents);
    for (int comp = 0; comp < numComponents; comp++) {
        char buf[128];
        sprintf(buf, "component_%d", comp);
        string label = "component_" + std::to_string(comp);
        string name = header.m_string[label];
        printf("\t%d: %s\n", comp, name.data());
    }

    if (argc == 1) {
        printf("no level max/component specified, exiting\n");
        handle.close();
        return 0;
    }

    int maxlevel = atoi(argv[1]);
    int component = atoi(argv[2]);

    fprintf(stderr, "maxLevel = %d, component = %d\n", maxlevel, component);

    // would like to use this, but chombo chokes
    Interval interval(component, component);

    IntVect size;
    float *brick;

    for (int lev = 0; lev < maxlevel; lev++) {
        Box domain;
        Real dx;
        Real dt;
        Real time;
        LevelData<FArrayBox> *data = new LevelData<FArrayBox>();

        int ref = 0;
        int err = readLevel(handle, lev, *data, dx, dt, time, domain,
                            ref, Interval(), true);
        if (err != 0) {
            printf("error reading level %d\n", lev);
            break;
        }

        printf("level %d: refinement: %d\n", lev, ref);

        const DisjointBoxLayout &layout = data->getBoxes();
        printf("boxes: %d\n", layout.size());

        if (lev == 0) {
            IntVect min, max;
            bool first = true;
            for (DataIterator dit = data->dataIterator(); dit.ok(); ++dit) {
                const Box box = data->box(dit);
                if (first) {
                    min = box.smallEnd();
                    max = box.bigEnd();
                    first = false;
                } else {
                    min.min(box.smallEnd());
                    max.max(box.bigEnd());
                }
            }
            cout << "L0: " << min << " -> " << max << endl;
            size = max - min + 1;
            brick = allocate(size);
        } else {
            brick = resize(brick, size);
        }

        cout << size << endl;

        for (DataIterator dit = data->dataIterator(); dit.ok(); ++dit) {
            const Box box = data->box(dit);
            cout << "\t" << box << endl;

            const FArrayBox &farray = (*data)[dit];
            for (int z = 0; z < box.size(2); z++) {
                for (int y = 0; y < box.size(1); y++) {
                    for (int x = 0; x < box.size(1); x++) {
                        IntVect iv(z, y, x);
                        iv += box.smallEnd();
                        Real val = farray.get(iv, component);
                        //                        printf("%f ", val);
                        setval(brick, val, size, iv);
                    }
                }
            }
            //            printf("\n");
        }

        delete data;
    }

    write(brick, size);
    delete [] brick;

    handle.close();

    return 0;
}

#if 0
int main(int argc, char **argv)
{
    Vector<DisjointBoxLayout> vectGrids;
    Vector<LevelData<FArrayBox>*> vectData;
    Vector<string> vectNames;
    Box domain;
    Real dx;
    Real dt;
    Real time;
    Vector<int> ratio;
    int levels = 1;

    int retVal = ReadAMRHierarchyHDF5("gear.hdf5",
                                      vectGrids, vectData, vectNames,
                                      domain, dx, dt, time, ratio,
                                      levels);

    fprintf(stderr, "%d\n", retVal);

    return 0;
}
#endif
