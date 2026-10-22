<script lang="ts">
	import { untrack } from 'svelte';
	import { CPU_GHZ_LABEL } from '$lib/constants';
	import { fmtDate } from '$lib/dates';
	import { fmt, fmtCycles } from '$lib/format';
	import { stripHtml } from '$lib/html';
	import type { VersionYaml } from '$lib/types';
	import {
		COMPARE_METRICS,
		FLAG_TITLES,
		METRIC_LABELS,
		compareVersions,
		formatChange,
		type CompareMode,
		type MetricValue,
		type ParamSetComparison,
		type VersionOption
	} from '$lib/versionCompare';

	let { versions, options }: { versions: VersionYaml[]; options: VersionOption[] } = $props();

	// The page wraps this component in {#key slug}, so reading props once at init is intended.
	let fromId = $state(untrack(() => options[1]?.id ?? options[0].id));
	let toId = $state(untrack(() => options[0].id));
	let mode = $state<CompareMode>('sizes');

	const indexOf = (id: string) =>
		Math.max(
			0,
			options.findIndex((o) => o.id === id)
		);
	const fromOpt = $derived(options[indexOf(fromId)]);
	const toOpt = $derived(options[indexOf(toId)]);
	const comparison = $derived(compareVersions(versions[indexOf(fromId)], versions[indexOf(toId)]));
	const metrics = $derived(COMPARE_METRICS[mode]);

	const WORDS = {
		sizes: { down: 'smaller', up: 'larger' },
		cycles: { down: 'faster', up: 'slower' }
	} as const;

	const optionLabel = (o: VersionOption) => `${o.label} · ${fmtDate(o.date)}`;
	const show = (v: MetricValue | null) => (v === null ? '—' : mode === 'sizes' ? fmt(v.value) : fmtCycles(v.value));

	function swap() {
		[fromId, toId] = [toId, fromId];
	}

	function flagLabel(r: ParamSetComparison): { text: string; title: string; tone: string } {
		if (r.flagChange === 'cleared' && r.fromFlag) {
			return {
				text: 'Flag cleared',
				title: `Was ${FLAG_TITLES[r.fromFlag.kind]}: ${stripHtml(r.fromFlag.html)}`,
				tone: 'text-pqs-steel dark:text-pqs-bluegray'
			};
		}
		const flag = r.toFlag ?? (r.presence === 'removed' ? r.fromFlag : null);
		if (!flag) return { text: '—', title: '', tone: 'text-pqs-bluegray' };
		const tone = flag.kind === 'broken' ? 'text-[#B91C1C] dark:text-[#EF4444]' : 'text-[#8a5208] dark:text-pqs-apricot';
		return { text: FLAG_TITLES[flag.kind], title: stripHtml(flag.html), tone };
	}

	const select =
		'h-11 min-w-0 max-w-full rounded-md border border-pqs-bluegray bg-white px-3 text-[15px] text-pqs-midnight dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-smoke';
	const toggle = 'px-3 py-1.5 font-heading text-sm font-semibold';
</script>

{#snippet valueCell(v: MetricValue | null)}
	{#if v?.estimated}
		<span class="underline decoration-pqs-scarlet decoration-wavy" title="Estimated from µs at {CPU_GHZ_LABEL}"
			>{show(v)}<span class="sr-only"> (estimated)</span></span
		>
	{:else}
		{show(v)}
	{/if}
{/snippet}

<section aria-labelledby="compare-heading" class="mt-10">
	<h2 id="compare-heading" class="font-heading text-xl font-bold text-pqs-midnight dark:text-pqs-smoke">
		Compare versions
	</h2>
	<p class="mt-1 text-sm text-pqs-steel dark:text-pqs-bluegray">Parameter sets are matched by name.</p>

	<div class="mt-4 flex flex-wrap items-end gap-3">
		<div class="flex flex-col gap-1">
			<label for="compare-from" class="font-heading text-xs font-semibold text-pqs-steel dark:text-pqs-bluegray"
				>From</label
			>
			<select id="compare-from" bind:value={fromId} class={select}>
				{#each options as o (o.id)}<option value={o.id}>{optionLabel(o)}</option>{/each}
			</select>
		</div>
		<button
			type="button"
			onclick={swap}
			aria-label="Swap From and To"
			class="h-11 rounded-md border border-pqs-bluegray px-3 hover:bg-pqs-ashgray focus-visible:outline-2 focus-visible:outline-pqs-apricot dark:border-pqs-steel dark:hover:bg-pqs-steel/40"
			><span aria-hidden="true">⇄</span></button
		>
		<div class="flex flex-col gap-1">
			<label for="compare-to" class="font-heading text-xs font-semibold text-pqs-steel dark:text-pqs-bluegray">To</label
			>
			<select id="compare-to" bind:value={toId} class={select}>
				{#each options as o (o.id)}<option value={o.id}>{optionLabel(o)}</option>{/each}
			</select>
		</div>
		<div
			role="group"
			aria-label="Compare"
			class="ml-auto inline-flex overflow-hidden rounded-md border border-pqs-bluegray dark:border-pqs-steel"
		>
			<button
				type="button"
				aria-pressed={mode === 'sizes'}
				onclick={() => (mode = 'sizes')}
				class="{toggle} {mode === 'sizes' ? 'bg-pqs-steel text-white' : 'text-pqs-steel dark:text-pqs-bluegray'}"
				>Sizes</button
			>
			<button
				type="button"
				aria-pressed={mode === 'cycles'}
				onclick={() => (mode = 'cycles')}
				class="{toggle} {mode === 'cycles' ? 'bg-pqs-steel text-white' : 'text-pqs-steel dark:text-pqs-bluegray'}"
				>Cycles</button
			>
		</div>
	</div>

	{#if fromId === toId}
		<p class="mt-3 text-sm text-pqs-steel dark:text-pqs-bluegray">
			From and To are the same version; pick two different versions to see changes.
		</p>
	{/if}

	<div class="relative mt-4 overflow-x-auto rounded border border-pqs-ashgray shadow-sm dark:border-pqs-steel">
		<table class="min-w-full text-sm">
			<caption class="sr-only">Change from {optionLabel(fromOpt)} to {optionLabel(toOpt)}</caption>
			<thead>
				<tr class="bg-pqs-steel font-heading text-xs text-white">
					<th scope="col" class="px-3 py-2.5 text-left font-semibold">Parameter set</th>
					<th scope="col" class="px-3 py-2.5 text-right font-semibold">Level</th>
					{#each metrics as key (key)}
						<th scope="col" class="px-3 py-2.5 text-right font-semibold whitespace-nowrap">{METRIC_LABELS[key]}</th>
						<th scope="col" class="px-3 py-2.5 text-right font-semibold">Change</th>
					{/each}
					<th scope="col" class="px-3 py-2.5 text-left font-semibold whitespace-nowrap"
						>Security flag <span class="font-normal">(today's assessment)</span></th
					>
				</tr>
			</thead>
			<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
				{#each comparison.rows as r (r.name)}
					{@const flag = flagLabel(r)}
					<tr>
						<th scope="row" class="px-3 py-1.5 text-left font-mono text-xs font-normal whitespace-nowrap">
							{r.name}
							{#if r.presence !== 'both'}
								<span
									class="ml-1 rounded bg-pqs-ashgray px-1.5 py-px font-heading text-[11px] font-bold text-pqs-midnight dark:bg-pqs-steel/40 dark:text-pqs-smoke"
									>{r.presence === 'added' ? `new in ${toOpt.label}` : `not in ${toOpt.label}`}</span
								>
							{/if}
						</th>
						<td class="px-3 py-1.5 text-right whitespace-nowrap text-pqs-steel tabular-nums dark:text-pqs-bluegray">
							{r.fromLevel && r.toLevel && r.fromLevel !== r.toLevel
								? `${r.fromLevel} → ${r.toLevel}`
								: (r.toLevel ?? r.fromLevel)}
						</td>
						{#each metrics as key (key)}
							{@const m = r.metrics[key]}
							{@const change = formatChange(m.pct)}
							<td class="px-3 py-1.5 text-right whitespace-nowrap tabular-nums">
								{@render valueCell(m.from)} <span aria-hidden="true">→</span><span class="sr-only"> to </span>
								{@render valueCell(m.to)}
							</td>
							<td
								class="px-3 py-1.5 text-right whitespace-nowrap tabular-nums {change.direction === 'down'
									? 'text-pqs-steel dark:text-pqs-bluegray'
									: change.direction === 'up'
										? 'text-[#8a5208] dark:text-pqs-apricot'
										: 'text-pqs-bluegray'}"
							>
								{#if change.symbol}<span aria-hidden="true">{change.symbol}</span
									>{' '}{/if}{change.text}{#if change.direction === 'down' || change.direction === 'up'}<span
										class="sr-only"
									>
										{WORDS[mode][change.direction]}</span
									>{/if}
							</td>
						{/each}
						<td
							class="px-3 py-1.5 font-heading text-xs font-semibold whitespace-nowrap {flag.tone}"
							title={flag.title || undefined}
							>{flag.text}{#if flag.title}<span class="sr-only">. {flag.title}</span>{/if}</td
						>
					</tr>
				{/each}
			</tbody>
		</table>
	</div>

	<p class="mt-2 text-xs text-pqs-steel dark:text-pqs-bluegray">
		▼ smaller or faster, ▲ larger or slower.
		{#if mode === 'sizes'}Sizes in bytes.{/if}
		Flags are today's assessment of each version's parameter sets; the history below shows when each attack was published.
	</p>
	{#if mode === 'cycles' && fromId !== toId}
		<p
			data-testid="perf-note"
			class="mt-2 rounded bg-[#fdf6e7] px-3 py-2 text-xs text-pqs-midnight dark:bg-[#33270a] dark:text-pqs-smoke"
		>
			Cycle counts as recorded for each version. {fromOpt.label} ({fmtDate(fromOpt.date)}): {comparison.perf.from ??
				'source not recorded'}.
			{toOpt.label} ({fmtDate(toOpt.date)}): {comparison.perf.to ?? 'source not recorded'}.
			{comparison.perf.same ? 'Both cite the same source, but' : 'The sources differ, and'} machines and implementations can
			differ between versions, so a change mixes the scheme's own changes with platform effects.
			{#if comparison.perf.anyEstimated}Wavy underline: estimated from µs at {CPU_GHZ_LABEL}.{/if}
		</p>
	{/if}
</section>
