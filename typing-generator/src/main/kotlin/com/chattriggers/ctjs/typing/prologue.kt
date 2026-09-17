package com.chattriggers.ctjs.typing

val manualRoots = setOf(
    "java.awt.Color",
    "java.util.ArrayList",
    "java.util.HashMap",
    "gg.essential.universal.UKeyboard",
    "net.minecraft.util.Hand",
    "org.spongepowered.asm.mixin.injection.callback.CallbackInfo",
)

private val providedTypes = requireNotNull(object {}.javaClass.getResourceAsStream("/provided-types.properties"))
    .bufferedReader()
    .useLines { lines ->
        lines.filter { it.isNotBlank() && !it.startsWith('#') }
            .associate { line -> line.substringBefore('=') to line.substringAfter('=').replace('$', '.') }
    }

val prologue = """
    /// <reference no-default-lib="true" />
    /// <reference lib="es2015" />
    export {};
    
    declare interface String {
      addFormatting(): string;
      addColor(): string;
      removeFormatting(): string;
      replaceFormatting(): string;
    }
    
    declare interface Number {
      easeOut(to: number, speed: number, jump: number): number;
      easeColor(to: number, speed: number, jump: number): java.awt.Color;
    }

    interface RegisterTypes {
      chat(...args: (string | unknown)[]): com.chattriggers.ctjs.api.triggers.ChatTrigger;
      actionBar(...args: (string | unknown)[]): com.chattriggers.ctjs.api.triggers.ChatTrigger;
      worldLoad(): com.chattriggers.ctjs.api.triggers.Trigger;
      worldUnload(): com.chattriggers.ctjs.api.triggers.Trigger;
      clicked(mouseX: number, mouseY: number, button: number, isPressed: boolean): com.chattriggers.ctjs.api.triggers.Trigger;
      scrolled(mouseX: number, mouseY: number, scrollDelta: number): com.chattriggers.ctjs.api.triggers.Trigger;
      dragged(mouseXDelta: number, mouseYDelta: number, mouseX: number, mouseY: number, mouseButton: number): com.chattriggers.ctjs.api.triggers.Trigger;
      soundPlay(position: com.chattriggers.ctjs.api.vec.Vec3f, name: string, volume: number, pitch: number, category: net.minecraft.sound.SoundCategory, event: org.spongepowered.asm.mixin.injection.callback.CallbackInfo): com.chattriggers.ctjs.api.triggers.SoundPlayTrigger;
      tick(ticksElapsed: number): com.chattriggers.ctjs.api.triggers.Trigger;
      step(stepsElapsed: number): com.chattriggers.ctjs.api.triggers.StepTrigger;
      renderWorld(partialTicks: number): com.chattriggers.ctjs.api.triggers.Trigger;
      preRenderWorld(partialTicks: number): com.chattriggers.ctjs.api.triggers.Trigger;
      postRenderWorld(partialTicks: number): com.chattriggers.ctjs.api.triggers.Trigger;
      renderOverlay(): com.chattriggers.ctjs.api.triggers.Trigger;
      renderPlayerList(event: org.spongepowered.asm.mixin.injection.callback.CallbackInfo): com.chattriggers.ctjs.api.triggers.EventTrigger;
      drawBlockHighlight(position: BlockPos, event: CancellableEvent): com.chattriggers.ctjs.api.triggers.EventTrigger;
      gameLoad(): com.chattriggers.ctjs.api.triggers.Trigger;
      gameUnload(): com.chattriggers.ctjs.api.triggers.Trigger;
      command(...args: string[]): com.chattriggers.ctjs.api.triggers.CommandTrigger;
      guiOpened(screen: net.minecraft.client.gui.screen.Screen, event: org.spongepowered.asm.mixin.injection.callback.CallbackInfo): com.chattriggers.ctjs.api.triggers.EventTrigger;
      guiClosed(screen: net.minecraft.client.gui.screen.Screen): com.chattriggers.ctjs.api.triggers.Trigger;
      dropItem(item: Item, entireStack: boolean, event: org.spongepowered.asm.mixin.injection.callback.CallbackInfo): com.chattriggers.ctjs.api.triggers.EventTrigger;
      messageSent(message: string, event: CancellableEvent): com.chattriggers.ctjs.api.triggers.EventTrigger;
      itemTooltip(lore: TextComponent[], item: Item, event: org.spongepowered.asm.mixin.injection.callback.CallbackInfo): com.chattriggers.ctjs.api.triggers.EventTrigger;
      playerInteract(interaction: com.chattriggers.ctjs.api.entity.PlayerInteraction, interactionTarget: Entity | Block | Item, event: CancellableEvent): com.chattriggers.ctjs.api.triggers.EventTrigger;
      entityDamage(entity: Entity, attacker: PlayerMP): com.chattriggers.ctjs.api.triggers.Trigger;
      entityDeath(entity: Entity): com.chattriggers.ctjs.api.triggers.Trigger;
      guiRender(mouseX: number, mouseY: number, screen: net.minecraft.client.gui.screen.Screen): com.chattriggers.ctjs.api.triggers.Trigger;
      guiKey(char: String, keyCode: number, screen: net.minecraft.client.gui.screen.Screen, event: CancellableEvent): com.chattriggers.ctjs.api.triggers.EventTrigger;
      guiMouseClick(mouseX: number, mouseY: number, mouseButton: number, isPressed: boolean, screen: net.minecraft.client.gui.screen.Screen, event: CancellableEvent): com.chattriggers.ctjs.api.triggers.EventTrigger;
      guiMouseDrag(mouseXDelta: number, mouseYDelta: number, mouseX: number, mouseY: number, mouseButton: number, screen: net.minecraft.client.gui.screen.Screen, event: CancellableEvent): com.chattriggers.ctjs.api.triggers.EventTrigger;
      packetSent(packet: net.minecraft.network.packet.Packet<unknown>, event: org.spongepowered.asm.mixin.injection.callback.CallbackInfo): com.chattriggers.ctjs.api.triggers.PacketTrigger;
      packetReceived(packet: net.minecraft.network.packet.Packet<unknown>, event: CancellableEvent): com.chattriggers.ctjs.api.triggers.PacketTrigger;
      serverConnect(): com.chattriggers.ctjs.api.triggers.Trigger;
      serverDisconnect(): com.chattriggers.ctjs.api.triggers.Trigger;
      renderEntity(entity: Entity, partialTicks: number, event: CancellableEvent): com.chattriggers.ctjs.api.triggers.RenderEntityTrigger;
      renderBlockEntity(blockEntity: BlockEntity, partialTicks: number, event: CancellableEvent): com.chattriggers.ctjs.api.triggers.RenderBlockEntityTrigger;
      postGuiRender(mouseX: number, mouseY: number, screen: net.minecraft.client.gui.screen.Screen): com.chattriggers.ctjs.api.triggers.Trigger;
      spawnParticle(particle: Particle, event: org.spongepowered.asm.mixin.injection.callback.CallbackInfo): com.chattriggers.ctjs.api.triggers.EventTrigger;
    }
  
    declare global {
      const Java: {
        /**
         * Returns the Java Class or Package given by name. If you want to
         * enforce the name is a class, use Java.class() instead.
         */
        type(name: string): java.lang.Package | java.lang.Class<any>;
  
        /**
         * Returns the Java Class given by `className`. Throws an error if the
         * name is not a valid class name.
         */
        class(className: string): java.lang.Class<any>;
      };

      /**
       * Runs `func` in a Java synchronized() block with `lock` as the synchronizer
       */
      function sync(func: () => void, lock: unknown): void;
  
      /**
       * Runs `func` after `delayInMs` milliseconds. A new thread is spawned to accomplish
       * this, which means this function is asynchronous. If you want to avoid the Thread
       * instantiation, use `Client.scheduleTask(delayInTicks, func)`.
       */
      function setTimeout(func: () => void, delayInMs: number): void;

      const ArrayList: typeof java.util.ArrayList;
      interface ArrayList<T> extends java.util.ArrayList<T> {}
      const HashMap: typeof java.util.HashMap;
      interface HashMap<K, V> extends java.util.HashMap<K, V> {}
      
${providedTypes.entries.filterNot { it.key == "ArrayList" || it.key == "HashMap" }.joinToString("") { (name, type) ->
"const $name: typeof $type;\ninterface $name extends $type {}\n"
}.prependIndent("      ")}

      /**
       * Registers a new trigger and returns it.
       */
      function register<T extends keyof RegisterTypes>(
        name: T, 
        cb: (...args: Parameters<RegisterTypes[T]>) => void,
      ): ReturnType<RegisterTypes[T]>;

      /**
       * Cancels the given event
       */
      function cancel(event: CancellableEvent | org.spongepowered.asm.mixin.injection.callback.CallbackInfo): void;

      /**
       * Creates a custom trigger. `name` can be used as the first argument of a
       * subsequent call to `register`. Returns an object that can be used to
       * invoke the trigger.
       */
      function createCustomTrigger(name: string): { trigger(...args: unknown[]) };
      
      function easeOut(start: number, finish: number, speed: number, jump?: number): number;
      function easeColor(start: number, finish: number, speed: number, jump?: number): java.awt.Color;

      function print(message: string, color?: java.awt.Color): void;
      function println(message: string, color?: java.awt.Color, end?: string): void;

      const console: {
        assert(condition: boolean, message: string): void;
        clear(): void;
        count(label?: string): void;
        debug(args: unknown[]): void;
        dir(obj: object): void;
        dirxml(obj: object): void;
        error(...args: unknown[]): void;
        group(...args: unknown[]): void;
        groupCollapsed(...args: unknown[]): void;
        groupEnd(...args: unknown[]): void;
        info(...args: unknown[]): void;
        log(...args: unknown[]): void;
        table(data: object, columns?: string[]): void;
        time(label?: string): void;
        timeEnd(label?: string): void;
        trace(...args: unknown[]): void;
        warn(...args: unknown[]): void;
      };
""".trimIndent()
