package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.V5Irc;
import com.chattriggers.ctjs.api.triggers.PacketEvent;
import net.minecraft.network.protocol.game.ServerboundChatPacket;
import com.chattriggers.ctjs.internal.engine.CTEvents;
import com.chattriggers.ctjs.api.triggers.TriggerType;
import io.netty.channel.ChannelFutureListener;
import io.netty.channel.ChannelHandlerContext;
import net.minecraft.network.Connection;
import net.minecraft.network.PacketListener;
import net.minecraft.network.protocol.PacketFlow;
import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.game.ClientboundBundlePacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Connection.class)
public abstract class ConnectionMixin {
    @Shadow
    public abstract PacketFlow getReceiving();

    @Inject(
        method = "channelRead0(Lio/netty/channel/ChannelHandlerContext;Lnet/minecraft/network/protocol/Packet;)V",
        at = @At(
            value = "INVOKE",
            target = "Lnet/minecraft/network/Connection;genericsFtw(Lnet/minecraft/network/protocol/Packet;Lnet/minecraft/network/PacketListener;)V",
            shift = At.Shift.BEFORE
        ),
        cancellable = true
    )
    private void injectHandlePacket(ChannelHandlerContext channelHandlerContext, Packet<?> packet, CallbackInfo ci) {
        if (getReceiving() == PacketFlow.CLIENTBOUND)
            CTEvents.PACKET_RECEIVED.invoker().receive(packet, ci);
    }

    @Inject(method = "genericsFtw", at = @At("HEAD"))
    private static void triggerPacketEvent(Packet<?> packet, PacketListener listener, CallbackInfo ci) {
        if (packet instanceof ClientboundBundlePacket bundlePacket) {
            bundlePacket.subPackets().forEach(PacketEvent.RECEIVE.invoker()::trigger);
        } else {
            PacketEvent.RECEIVE.invoker().trigger(packet);
        }
    }

    @Inject(
        method = "send(Lnet/minecraft/network/protocol/Packet;Lio/netty/channel/ChannelFutureListener;)V",
        at = @At("HEAD"),
        cancellable = true
    )
    private void injectSendPacket(Packet<?> packet, ChannelFutureListener channelFutureListener, CallbackInfo ci) {
        if (packet instanceof ServerboundChatPacket chat && chat.message().startsWith("#")) {
            V5Irc.send(chat.message().substring(1));
            ci.cancel();
            return;
        }
        TriggerType.PACKET_SENT.triggerAll(packet, ci);
    }
}
