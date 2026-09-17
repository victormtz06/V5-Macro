package com.chattriggers.ctjs.internal.mixins.sound;

import net.minecraft.client.sounds.ChannelAccess;
import net.minecraft.client.resources.sounds.SoundInstance;
import net.minecraft.client.sounds.SoundEngine;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
import org.spongepowered.asm.mixin.gen.Invoker;

import java.util.Map;

@Mixin(SoundEngine.class)
public interface SoundEngineAccessor {
    @Accessor
    ChannelAccess getChannelAccess();

    @Accessor
    Map<SoundInstance, ChannelAccess.ChannelHandle> getInstanceToChannel();

    @Invoker
    void invokeLoadLibrary();

    @Invoker
    void invokeTickInGameSound();
}
