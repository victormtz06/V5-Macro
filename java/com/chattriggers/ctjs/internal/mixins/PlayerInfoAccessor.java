package com.chattriggers.ctjs.internal.mixins;

import net.minecraft.client.multiplayer.PlayerInfo;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;

@Mixin(PlayerInfo.class)
public interface PlayerInfoAccessor {
    @Invoker
    void invokeSetLatency(int latency);
}
