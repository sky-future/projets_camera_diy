#pragma once

#include "DownscaleStrategies/Center.h"
#include "DownscaleStrategies/Cross.h"

namespace Eloquent {
    namespace Vision {
        namespace ImageProcessing {
            namespace Downscale {

                /**
                 *
                 * @tparam sourceWidth
                 * @tparam sourceHeight
                 * @tparam channels
                 */
                template<uint16_t sourceWidth, uint16_t sourceHeight, uint8_t channels, uint8_t destWidth, uint8_t destHeight>
                class Downscaler {
                public:
                    /**
                     *
                     * @param strategy
                     */
                    Downscaler(DownscaleStrategies::Strategy<sourceWidth, sourceHeight, destWidth, destHeight> *strategy) :
                            _strategy(strategy) {

                    }

                    /**
                     * Downscale image according to the given strategy
                     * @param source
                     * @param dest
                     */
                    void downscale(uint8_t *source, uint8_t *dest ) {
                        const uint8_t blockSizeX = sourceWidth / destWidth *3; // *3 is relative to the RGB888. if we inject Grayscale at MotionDetection, we have to put *1
                        const uint8_t blockSizeY = sourceHeight / destHeight;
                        const uint32_t blockRowsOffset = blockSizeY * sourceWidth;
                        uint32_t offsetY = 0;
                        uint8_t block[blockSizeY][blockSizeX];
                        for (uint16_t destY = 0; destY < destHeight; destY++) {
                            for (uint16_t destX = 0; destX < destWidth; destX++) {
                                uint32_t offset = offsetY *3 + destX * blockSizeX;

                                // fill the block
                                // @todo select channel
                                for (uint8_t y = 0; y < blockSizeY; y++) {
                                    memcpy(block[y], source + offset, blockSizeX); // *3 is relative to the RGB888. if we inject Grayscale at MotionDetection, we have to put *1
                                    offset += sourceWidth;
                                }
                                dest[destY*destWidth + destX] = _strategy->apply(block);
                            }

                            offsetY += blockRowsOffset;
                        }
                    }

                protected:
                    /**
                     * Strategy for downscale
                     */
                    DownscaleStrategies::Strategy<sourceWidth, sourceHeight, destWidth, destHeight> *_strategy;
                };
            }
        }
    }
}