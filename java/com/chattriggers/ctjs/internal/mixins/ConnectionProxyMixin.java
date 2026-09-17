package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Proxy;
import com.chattriggers.ctjs.api.client.ProxyInfo;
import io.netty.channel.ChannelPipeline;
import io.netty.handler.proxy.Socks5ProxyHandler;
import java.net.InetSocketAddress;
import java.util.List;
import net.minecraft.network.BandwidthDebugMonitor;
import net.minecraft.network.Connection;
import net.minecraft.network.protocol.PacketFlow;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Connection.class)
public abstract class ConnectionProxyMixin {

    @Inject(method = "configureSerialization", at = @At("HEAD"))
    private static void addHandlers(
            ChannelPipeline pipeline,
            PacketFlow nwside,
            boolean singleplayer,
            BandwidthDebugMonitor packetSizeLogger,
            CallbackInfo ci
    ) {
        if (nwside != PacketFlow.CLIENTBOUND || singleplayer) return;

        List<Proxy> activeProxies = ProxyInfo.INSTANCE.getEnabledProxies();
        if (activeProxies.isEmpty()) return;

        Proxy proxy = activeProxies.getFirst();

        String ip = proxy.getIp();
        if (ip == null) return;
        ip = ip.trim();

        String username = proxy.getUsername();
        String password = proxy.getPassword();

        username = (username != null) ? username.trim() : "";
        password = (password != null) ? password.trim() : "";

        InetSocketAddress address = new InetSocketAddress(ip, proxy.getPort());
        Socks5ProxyHandler handler = username.isEmpty()
                ? new Socks5ProxyHandler(address)
                : new Socks5ProxyHandler(address, username, password);
        pipeline.addFirst("proxy", handler);
    }
}
