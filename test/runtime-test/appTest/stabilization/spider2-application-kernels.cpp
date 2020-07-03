/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2020)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
 *
 * Spider is a dataflow based runtime used to execute dynamic PiSDF
 * applications. The Preesm tool may be used to design PiSDF applications.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

/* === Include(s) === */

#include "spider2-application.h"

/* === Function(s) declaration === */

void spider::createUserApplicationKernels() {
    /* == Register READYUV kernel == */
    spider::api::createRuntimeKernel(readyuvRTKernel);

    /* == Register YUVDISPLAY kernel == */
    spider::api::createRuntimeKernel(yuvdisplayRTKernel);

    /* == Register YUVWRITE kernel == */
    spider::api::createRuntimeKernel(yuvwriteRTKernel);

    /* == Register FINDDOMINATINGMOTIONVECTOR kernel == */
    spider::api::createRuntimeKernel(finddominatingmotionvectorRTKernel);

    /* == Register RENDERFRAME kernel == */
    spider::api::createRuntimeKernel(renderframeRTKernel);

    /* == Register ACCUMULATEMOTION kernel == */
    spider::api::createRuntimeKernel(accumulatemotionRTKernel);

    /* == Register DIVIDEBLOCKS kernel == */
    spider::api::createRuntimeKernel(divideblocksRTKernel);

    /* == Register COMPUTEBLOCKMOTIONVECTOR kernel == */
    spider::api::createRuntimeKernel(computeblockmotionvectorRTKernel);
}
    
/* === readYUV === */
    
void spider::readyuvRTKernel(const int64_t [], int64_t [], void *[], void *[]) {
    fprintf(stderr, "[stabilization] read success!\n");
}
    
/* === yuvDisplay === */
    
void spider::yuvdisplayRTKernel(const int64_t [], int64_t [], void *[], void *[]) {
    fprintf(stderr, "[stabilization] display success!\n");
}
    
/* === yuvWrite === */
    
void spider::yuvwriteRTKernel(const int64_t [], int64_t [], void *[], void *[]) {
    fprintf(stderr, "[stabilization] write success!\n");
}
    
/* === findDominatingMotionVector === */
    
void spider::finddominatingmotionvectorRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    findDominatingMotionVector(
    /* = nbVectors        = */ static_cast<int>(inputParams[0]),
    /* = vectors          = */ reinterpret_cast<const coord *>(inputs[0]),
    /* = dominatingVector = */ reinterpret_cast<coordf *>(outputs[0]));
}
    
/* === renderFrame === */
    
void spider::renderframeRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    renderFrame(
    /* = frameWidth  = */ static_cast<int>(inputParams[0]),
    /* = frameHeight = */ static_cast<int>(inputParams[1]),
    /* = dispWidth   = */ static_cast<int>(inputParams[2]),
    /* = dispHeight  = */ static_cast<int>(inputParams[3]),
    /* = delta       = */ reinterpret_cast<const coordf *>(inputs[0]),
    /* = deltaPrev   = */ reinterpret_cast<const coordf *>(inputs[1]),
    /* = yIn         = */ reinterpret_cast<const unsigned char *>(inputs[2]),
    /* = uIn         = */ reinterpret_cast<const unsigned char *>(inputs[3]),
    /* = vIn         = */ reinterpret_cast<const unsigned char *>(inputs[4]),
    /* = yPrev       = */ reinterpret_cast<const unsigned char *>(inputs[5]),
    /* = uPrev       = */ reinterpret_cast<const unsigned char *>(inputs[6]),
    /* = vPrev       = */ reinterpret_cast<const unsigned char *>(inputs[7]),
    /* = yOut        = */ reinterpret_cast<unsigned char *>(outputs[0]),
    /* = uOut        = */ reinterpret_cast<unsigned char *>(outputs[1]),
    /* = vOut        = */ reinterpret_cast<unsigned char *>(outputs[2]));
}
    
/* === accumulateMotion === */
    
void spider::accumulatemotionRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]) {
    accumulateMotion(
    /* = motionVector         = */ reinterpret_cast<const coordf *>(inputs[0]),
    /* = accumulatedMotionIn  = */ reinterpret_cast<const coordf *>(inputs[1]),
    /* = filteredMotionIn     = */ reinterpret_cast<coordf *>(inputs[2]),
    /* = filteredMotionOut    = */ reinterpret_cast<coordf *>(outputs[0]),
    /* = accumulatedMotionOut = */ reinterpret_cast<coordf *>(outputs[1]));
}
    
/* === divideBlocks === */
    
void spider::divideblocksRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    divideBlocks(
    /* = width       = */ static_cast<int>(inputParams[0]),
    /* = height      = */ static_cast<int>(inputParams[1]),
    /* = blockWidth  = */ static_cast<int>(inputParams[2]),
    /* = blockHeight = */ static_cast<int>(inputParams[3]),
    /* = frame       = */ reinterpret_cast<const unsigned char *>(inputs[0]),
    /* = blocksCoord = */ reinterpret_cast<coord *>(outputs[0]),
    /* = blocksData  = */ reinterpret_cast<unsigned char *>(outputs[1]));
}
    
/* === computeBlockMotionVector === */
    
void spider::computeblockmotionvectorRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    computeBlockMotionVector(
    /* = width         = */ static_cast<int>(inputParams[0]),
    /* = height        = */ static_cast<int>(inputParams[1]),
    /* = blockWidth    = */ static_cast<int>(inputParams[2]),
    /* = blockHeight   = */ static_cast<int>(inputParams[3]),
    /* = maxDeltaX     = */ static_cast<int>(inputParams[4]),
    /* = maxDeltaY     = */ static_cast<int>(inputParams[5]),
    /* = blockCoord    = */ reinterpret_cast<const coord *>(inputs[0]),
    /* = blockData     = */ reinterpret_cast<const unsigned char *>(inputs[1]),
    /* = previousFrame = */ reinterpret_cast<const unsigned char *>(inputs[2]),
    /* = vector        = */ reinterpret_cast<coord *>(outputs[0]));
}
