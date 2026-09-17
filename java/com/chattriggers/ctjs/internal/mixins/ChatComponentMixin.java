package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.chattriggers.ctjs.api.message.ChatLib;
import net.minecraft.client.gui.components.ChatComponent;
import net.minecraft.client.multiplayer.chat.GuiMessage;
import net.minecraft.client.multiplayer.chat.GuiMessageTag;
import net.minecraft.network.chat.MessageSignature;
import net.minecraft.network.chat.Component;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyVariable;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

import java.util.List;

@Mixin(ChatComponent.class)
public class ChatComponentMixin {
    @Final
    @Shadow
    private List<GuiMessage> allMessages;

    @ModifyVariable(
        method = "addMessage(Lnet/minecraft/network/chat/Component;Lnet/minecraft/network/chat/MessageSignature;Lnet/minecraft/client/multiplayer/chat/GuiMessageSource;Lnet/minecraft/client/multiplayer/chat/GuiMessageTag;)V",
        at = @At("HEAD"),
        ordinal = 0,
        argsOnly = true
    )
    private Component v5$addMessage(Component original) {
        return Client.processName(original);
    }

    @Inject(method = "clearMessages", at = @At("TAIL"))
    private void injectClear(boolean clearHistory, CallbackInfo ci) {
        ChatLib.INSTANCE.onChatHudClearChat$ctjs();
    }

    // TODO: is it this or addVisibleMessage
    @Inject(
        method = "addMessageToQueue(Lnet/minecraft/client/multiplayer/chat/GuiMessage;)V",
        at = @At(
            value = "INVOKE",
            target = "Ljava/util/List;removeLast()Ljava/lang/Object;",
            shift = At.Shift.BEFORE
        )
    )
    private void injectMessageRemovedForChatLimit(GuiMessage message, CallbackInfo ci) {
        ChatLib.INSTANCE.onChatHudLineRemoved$ctjs(allMessages.getLast());
    }

    // Note: ChatHudLine objects are also removed in queueForRemoval, however those are signature based.
    //       The Message objects that CT sends will always create ChatHudLine objects with null signatures,
    //       so objects removed in that method will never be in the ChatLib.chatLineIds map
}
