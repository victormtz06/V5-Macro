package com.chattriggers.ctjs.internal.mixins;

import net.minecraft.world.scores.ScoreHolder;
import net.minecraft.world.scores.Score;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(targets = "net.minecraft.world.scores.Scoreboard$1")
public interface Scoreboard$1Accessor {
    @Accessor("val$score")
    Score getScore();

    @Accessor("val$scoreHolder")
    ScoreHolder getHolder();
}
