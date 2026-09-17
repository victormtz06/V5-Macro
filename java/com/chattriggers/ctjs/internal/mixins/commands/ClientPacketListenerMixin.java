package com.chattriggers.ctjs.internal.mixins.commands;

import com.chattriggers.ctjs.internal.engine.CTEvents;
import com.mojang.brigadier.CommandDispatcher;
import net.fabricmc.fabric.api.client.command.v2.FabricClientCommandSource;
import net.minecraft.client.multiplayer.ClientPacketListener;
import net.minecraft.commands.SharedSuggestionProvider;
import net.minecraft.network.protocol.game.ClientboundCommandsPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(ClientPacketListener.class)
public class ClientPacketListenerMixin {
    @Shadow
    private CommandDispatcher<SharedSuggestionProvider> commands;

    @SuppressWarnings("unchecked")
    @Inject(method = "handleCommands", at = @At("TAIL"))
    private void injectOnCommandTree(ClientboundCommandsPacket packet, CallbackInfo ci) {
        CTEvents.NETWORK_COMMAND_DISPATCHER_REGISTER.invoker().register(
            (CommandDispatcher<FabricClientCommandSource>) (Object) commands
        );
    }
}
