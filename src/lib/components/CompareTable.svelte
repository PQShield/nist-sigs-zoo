<script lang="ts" generics="R extends CompareRow, G, F extends CompareFilter">
	import { base } from '$app/paths';
	import { slugOf } from '$lib/schemeIndex';
	import { schemeHref } from '$lib/slugs';
	import { programmeChips, securityBadges, type BadgeKind } from '$lib/badges';
	import { sortCompareRows, type CompareConfig, type CompareRow } from '$lib/compare';
	import type { CompareFilter, SortDir } from '$lib/types';
	import { allProgrammes } from '$lib/programmeData';
	import { CPU_GHZ_LABEL } from '$lib/constants';
	import { fmt, fmtCycles, fmtTime } from '$lib/format';
	import { displayUs } from '$lib/timing';
	import BadgeIcon from './BadgeIcon.svelte';
	import ProgrammeChips from './ProgrammeChips.svelte';
	import TableLegend from './TableLegend.svelte';

	interface Props {
		config: CompareConfig<R, G, F>;
		rows: R[];
		sortCol: string;
		sortDir: SortDir;
		onsort: (col: string, dir: SortDir) => void;
	}
	let { config, rows, sortCol, sortDir, onsort }: Props = $props();

	const idPrefix = $props.id();

	/** Sortable columns; the Programmes column (not sortable) is inserted before index 2. */
	const columns = $derived([
		{ key: 'scheme', label: 'Scheme', numeric: false },
		{ key: 'category', label: 'Category', numeric: false },
		{ key: 'parameterset', label: 'Parameter Set', numeric: false },
		{ key: 'level', label: 'Level', numeric: true },
		...config.table.columns.map((c) => ({ key: c.key, label: c.label, numeric: true }))
	]);
	const colspan = $derived(columns.length + 2); // + Programmes + Assumption

	const sortedRows = $derived(sortCompareRows(config.filter, rows, sortCol, sortDir));

	const DETAIL_TONE: Record<BadgeKind, string> = {
		'pre-quantum': 'bg-pqs-smoke dark:bg-pqs-steel/20',
		broken: 'bg-[#fdf2f2] dark:bg-[#3b1219]',
		implementation: 'bg-[#fff4ec] dark:bg-[#3a1d0c]',
		warning: 'bg-[#fdf6e7] dark:bg-[#33270a]',
		note: 'bg-pqs-smoke dark:bg-pqs-steel/20'
	};

	/** Which badge's detail row is open, per parameter set. */
	let open = $state<Record<string, BadgeKind | undefined>>({});

	function rowKey(row: R): string {
		return `${row.scheme}/${row.parameterset}`;
	}

	function shortParam(row: R): string {
		const prefix = row.scheme + '-';
		return row.parameterset.startsWith(prefix) ? row.parameterset.slice(prefix.length) : row.parameterset;
	}

	function toggle(key: string, kind: BadgeKind) {
		open[key] = open[key] === kind ? undefined : kind;
	}

	function setSort(col: string) {
		onsort(col, sortCol === col && sortDir === 'asc' ? 'desc' : 'asc');
	}

	function sortIndicator(col: string): string {
		if (sortCol !== col) return '';
		return sortDir === 'asc' ? ' ▲' : ' ▼';
	}

	function ariaSort(col: string): 'ascending' | 'descending' | undefined {
		if (sortCol !== col) return undefined;
		return sortDir === 'asc' ? 'ascending' : 'descending';
	}
</script>

{#snippet timeCell(us: number | null, cyc: number)}
	{@const shown = displayUs(us, cyc)}
	<td class="whitespace-nowrap px-2 py-1.5 text-right tabular-nums">
		{#if us != null}
			{fmtTime(us)}
		{:else if shown != null}
			<span
				class="underline decoration-wavy decoration-pqs-scarlet"
				title="Estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL}"
			>{fmtTime(shown)}<span class="sr-only"> (estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL})</span></span>
		{:else}
			<span class="text-pqs-bluegray" title="Not yet benchmarked">—<span class="sr-only"> not yet benchmarked</span></span>
		{/if}
	</td>
{/snippet}

<div class="overflow-hidden rounded border border-pqs-ashgray shadow-sm dark:border-pqs-steel">
	<div class="relative overflow-x-auto">
		<table class="min-w-full text-sm">
			<thead class="sticky top-0 z-10">
				<tr class="bg-pqs-steel font-heading text-xs text-white">
					{#each columns as col, i (col.key)}
						{#if i === 2}
							<th scope="col" class="whitespace-nowrap px-2 py-2.5 text-left font-semibold">Programmes</th>
						{/if}
						<th scope="col" aria-sort={ariaSort(col.key)} class="whitespace-nowrap p-0">
							<button
								type="button"
								onclick={() => setSort(col.key)}
								class="w-full cursor-pointer select-none px-2 py-2.5 font-semibold hover:bg-pqs-steel-light {col.numeric ? 'text-right' : 'text-left'}"
							>
								{col.label}{sortIndicator(col.key)}
							</button>
						</th>
					{/each}
					<th scope="col" class="px-2 py-2.5 text-left text-xs font-semibold">Assumption</th>
				</tr>
			</thead>
			<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
				{#each sortedRows as row, i (rowKey(row))}
					{@const key = rowKey(row)}
					{@const badges = securityBadges(row)}
					{@const openBadge = badges.find((b) => b.kind === open[key])}
					{@const detailId = `${idPrefix}-detail-${i}`}
					<tr class="hover:bg-pqs-smoke dark:hover:bg-pqs-steel/30">
						<td class="whitespace-nowrap px-2 py-1.5">
							<a
								href={schemeHref(base, slugOf(row.scheme))}
								class="font-heading font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot dark:hover:text-pqs-apricot-light"
							>{row.scheme}</a>
							{#each badges as badge (badge.kind)}
								<button
									type="button"
									class="ml-1 inline-grid h-6 w-6 place-items-center rounded align-middle hover:bg-pqs-ashgray focus-visible:outline-2 focus-visible:outline-pqs-apricot dark:hover:bg-pqs-steel/40"
									aria-label="{badge.title}: show details"
									aria-expanded={open[key] === badge.kind}
									aria-controls={detailId}
									onclick={() => toggle(key, badge.kind)}
								><BadgeIcon kind={badge.kind} /></button>
							{/each}
						</td>
						<td class="whitespace-nowrap px-2 py-1.5 text-pqs-steel dark:text-pqs-bluegray">{row.category}</td>
						<td class="px-2 py-1.5"><ProgrammeChips chips={programmeChips(row, allProgrammes)} /></td>
						{#if config.table.shortParamSet}
							<td class="whitespace-nowrap px-2 py-1.5 font-mono text-xs" title={row.parameterset}>{shortParam(row)}</td>
						{:else}
							<td class="whitespace-nowrap px-2 py-1.5 font-mono text-xs">{row.parameterset}</td>
						{/if}
						<td class="px-2 py-1.5 text-right tabular-nums text-pqs-steel dark:text-pqs-bluegray">
							{#if row.level !== null}
								{row.level === 'Pre-Quantum' ? 'Pre-quantum' : row.level}
							{:else if row.ngccLevel !== null}
								<span class="font-semibold text-[#6b21a8] dark:text-[#d8b4fe]" title="China NGCC target in classical bits; not a NIST level">{row.ngccLevel}-bit</span>
							{:else}
								—
							{/if}
						</td>
						{#each config.table.columns as col (col.key)}
							{#if col.time}
								{@const [us, cycles] = col.time(row)}
								{@render timeCell(us, cycles)}
							{:else if col.size}
								<td class="px-2 py-1.5 text-right tabular-nums">{fmt(col.size(row))}</td>
							{/if}
						{/each}
						<td class="max-w-48 whitespace-normal px-2 py-1.5 text-pqs-steel/80 dark:text-pqs-bluegray">{row.assumption}</td>
					</tr>
					{#if badges.length > 0}
						<tr id={detailId} hidden={!openBadge} class={openBadge ? DETAIL_TONE[openBadge.kind] : ''}>
							<td colspan={colspan} class="whitespace-normal px-3 py-2.5 pl-9 text-sm leading-relaxed text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-apricot [&_a]:underline">
								<div class="sticky left-0 max-w-[calc(100vw-3rem)]">
								{#if openBadge}
									<strong class="font-heading">{openBadge.title}:</strong>
									{@html openBadge.html}
									{#if openBadge.href && /^https?:\/\//.test(openBadge.href)}
										<a href={openBadge.href} target="_blank" rel="noopener noreferrer">Full findings report<span class="sr-only"> (opens in a new tab)</span></a>
									{/if}
								{/if}
								</div>
							</td>
						</tr>
					{/if}
				{/each}
				{#if sortedRows.length === 0}
					<tr>
						<td colspan={colspan} class="px-3 py-10 text-center text-pqs-bluegray">{config.table.emptyText}</td>
					</tr>
				{/if}
			</tbody>
		</table>
	</div>
	<div class="border-t border-pqs-ashgray bg-pqs-smoke px-3 py-1.5 font-heading text-xs text-pqs-steel dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray">
		<span>{sortedRows.length} parameter set{sortedRows.length === 1 ? '' : 's'}</span>
	</div>
	<TableLegend />
</div>
