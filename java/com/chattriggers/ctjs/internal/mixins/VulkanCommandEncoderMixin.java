//? if >=26.2 {
package com.chattriggers.ctjs.internal.mixins;

import com.mojang.blaze3d.vulkan.VulkanCommandEncoder;
import org.lwjgl.vulkan.VkCommandBuffer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;

@Mixin(VulkanCommandEncoder.class)
public interface VulkanCommandEncoderMixin {
    @Invoker("commandBuffer")
    VkCommandBuffer ctjs$getCommandBuffer();
}
//?}
