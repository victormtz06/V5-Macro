//? if >=26.2 {
package com.chattriggers.ctjs.internal.mixins;

import com.mojang.blaze3d.shaders.ShaderSource;
import com.mojang.blaze3d.vulkan.VulkanDevice;
import com.mojang.blaze3d.vulkan.VulkanInstance;
import com.mojang.blaze3d.vulkan.VulkanPhysicalDevice;
import com.mojang.blaze3d.vulkan.checkpoints.CheckpointExtension;
import com.chattriggers.ctjs.internal.accessors.VulkanDeviceAccessor;
import org.lwjgl.vulkan.VkDevice;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(VulkanDevice.class)
public class VulkanDeviceMixin implements VulkanDeviceAccessor {
    @Unique
    private VulkanPhysicalDevice ctjs$physicalDevice;

    @Inject(method = "<init>", at = @At("RETURN"))
    private void ctjs$capturePhysicalDevice(
            ShaderSource shaderSource,
            VulkanInstance instance,
            VulkanPhysicalDevice physicalDevice,
            java.util.Set<String> enabledExtensions,
            VkDevice device,
            long vma,
            CheckpointExtension checkpointExtension,
            CallbackInfo ci
    ) {
        ctjs$physicalDevice = physicalDevice;
    }

    @Override
    public VulkanPhysicalDevice ctjs$getPhysicalDevice() {
        return ctjs$physicalDevice;
    }
}
//?}
