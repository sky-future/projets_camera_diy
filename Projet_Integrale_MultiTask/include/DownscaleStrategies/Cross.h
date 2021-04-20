#pragma once

#include "Strategy.h"

namespace Eloquent {
    namespace Vision {
        namespace ImageProcessing {
            namespace DownscaleStrategies {

                /**
                 * Return the average of vertical and horizontal central pixels
                 * @tparam sourceWidth
                 * @tparam sourceHeight
                 * @tparam destWidth
                 * @tparam destHeight
                 */
                template <uint16_t sourceWidth, uint16_t sourceHeight, uint8_t destWidth, uint8_t destHeight>
                class Cross : public Strategy<sourceWidth, sourceHeight, destWidth, destHeight> {
                public:

                    /**
                     * Computing function
                     * @param block
                     * @return
                     */
                    const uint8_t apply(uint8_t block[sourceHeight / destHeight][sourceWidth / destWidth *3]) { // *3 is relative to the RGB888. if we inject Grayscale at MotionDetection, we have to put *1
                        const uint8_t blockSizeX = sourceWidth / destWidth *3; // *3 is relative to the RGB888. if we inject Grayscale at MotionDetection, we have to put *1
                        const uint8_t blockSizeY = sourceHeight / destHeight;
                        const uint8_t centerX = blockSizeX * 0.5;
                        const uint8_t centerY = blockSizeY * 0.5;
                        uint32_t accumulator = 0;

                        for (uint8_t y = 0; y < blockSizeY; y++)
                            accumulator += rgb888_to_gray(&block[y][centerX]); // rgb888_to_gray is relative to RGB888. if we inject Grayscale at MotionDetection, we need to remove the function as well as the pointer.

                        for (uint8_t x = 0; x < blockSizeX; x++)
                            accumulator += rgb888_to_gray(&block[centerY][x]);

                        return (accumulator - rgb888_to_gray(&block[centerY][centerX])) / (blockSizeX + blockSizeY - 1);
                    }
                };
            }
        }
    }
}