<script lang="ts">
	import { programmeChips, securityBadges, type BadgeKind } from '$lib/badges';
	import type { KemParameterSet, KemSortableColumn } from '$lib/types';
	import { allProgrammes } from '$lib/programmeData';
	import { CYCLES_PER_US, CPU_GHZ_LABEL } from '$lib/constants';
	import { fmt, fmtCycles, fmtTime } from '$lib/format';
	import { kemTimes } from '$lib/timing';
	import BadgeIcon from './BadgeIcon.svelte';
	import ProgrammeChips from './ProgrammeChips.svelte';
	import TableLegend from './TableLegend.svelte';

	let { rows, recommendedOnly }: { rows: KemParameterSet[]; recommendedOnly: boolean } = $props();

	const idPrefix = $props.id();

	let sortCol = $state<KemSortableColumn>('pkPlusCt');
	let sortDir = $state<'asc' | 'desc'>('asc');

	function shortParam(row: KemParameterSet): string {
		const prefix = row.scheme + '-';
		return row.parameterset.startsWith(prefix) ? row.parameterset.slice(prefix.length) : row.parameterset;
	}

	/** Sortable columns; the Programmes column (not sortable) is inserted before index 2. */
	const COLUMNS: { key: KemSortableColumn; label: string; numeric?: boolean }[] = [
		{ key: 'scheme', label: 'Scheme' },
		{ key: 'category', label: 'Category' },
		{ key: 'parameterset', label: 'Parameter Set' },
		{ key: 'level', label: 'Level', numeric: true },
		{ key: 'pk', label: 'pk (B)', numeric: true },
		{ key: 'ct', label: 'ct (B)', numeric: true },
		{ key: 'pkPlusCt', label: 'pk+ct (B)', numeric: true },
		{ key: 'keygenUs', label: 'Keygen', numeric: true },
		{ key: 'encapsUs', label: 'Encaps', numeric: true },
		{ key: 'decapsUs', label: 'Decaps', numeric: true }
	];
	const COLSPAN = COLUMNS.length + 2; // + Programmes + Assumption

	const DETAIL_TONE: Record<BadgeKind, string> = {
		'pre-quantum': 'bg-pqs-smoke dark:bg-pqs-steel/20',
		broken: 'bg-[#fdf2f2] dark:bg-[#3b1219]',
		implementation: 'bg-[#fff4ec] dark:bg-[#3a1d0c]',
		warning: 'bg-[#fdf6e7] dark:bg-[#33270a]',
		note: 'bg-pqs-smoke dark:bg-pqs-steel/20'
	};

	const levelOrder: Record<string, number> = { 'Pre-Quantum': 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5 };

	// Displayed time in µs, or null when the row has no timing data (shown as "—").
	const TIME_VALUE: Partial<Record<KemSortableColumn, (r: KemParameterSet) => number | null>> = {
		keygenUs: (r) => kemTimes(r).keygen,
		encapsUs: (r) => kemTimes(r).encaps,
		decapsUs: (r) => kemTimes(r).decaps
	};

	function compare(a: KemParameterSet, b: KemParameterSet, col: KemSortableColumn): number {
		switch (col) {
			case 'level':
				return (levelOrder[String(a.level)] ?? 99) - (levelOrder[String(b.level)] ?? 99);
			case 'pk':
				return a.pk - b.pk;
			case 'ct':
				return a.ct - b.ct;
			case 'pkPlusCt':
				return a.pkPlusCt - b.pkPlusCt;
			default: {
				const av = String(a[col] ?? '').toLowerCase();
				const bv = String(b[col] ?? '').toLowerCase();
				return av < bv ? -1 : av > bv ? 1 : 0;
			}
		}
	}

	const sortedRows = $derived(
		[...rows].sort((a, b) => {
			// Time columns keep rows without data last regardless of direction.
			const timeValue = TIME_VALUE[sortCol];
			if (timeValue) {
				const av = timeValue(a);
				const bv = timeValue(b);
				if (av == null && bv == null) return 0;
				if (av == null) return 1;
				if (bv == null) return -1;
				return sortDir === 'asc' ? av - bv : bv - av;
			}
			const cmp = compare(a, b, sortCol);
			return sortDir === 'asc' ? cmp : -cmp;
		})
	);

	/** Which badge's detail row is open, per parameter set. */
	let open = $state<Record<string, BadgeKind | undefined>>({});

	function rowKey(row: KemParameterSet): string {
		return `${row.scheme}/${row.parameterset}`;
	}

	function toggle(key: string, kind: BadgeKind) {
		open[key] = open[key] === kind ? undefined : kind;
	}

	function setSort(col: KemSortableColumn) {
		if (sortCol === col) {
			sortDir = sortDir === 'asc' ? 'desc' : 'asc';
		} else {
			sortCol = col;
			sortDir = 'asc';
		}
	}

	function sortIndicator(col: KemSortableColumn): string {
		if (sortCol !== col) return '';
		return sortDir === 'asc' ? ' ▲' : ' ▼';
	}

	function ariaSort(col: KemSortableColumn): 'ascending' | 'descending' | undefined {
		if (sortCol !== col) return undefined;
		return sortDir === 'asc' ? 'ascending' : 'descending';
	}
</script>

{#snippet timeCell(us: number | null, cyc: number)}
	<td class="whitespace-nowrap px-3 py-1.5 text-right tabular-nums">
		{#if us != null}
			{fmtTime(us)}
		{:else if cyc > 0}
			<span
				class="underline decoration-wavy decoration-pqs-scarlet"
				title="Estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL}"
			>{fmtTime(cyc / CYCLES_PER_US)}<span class="sr-only"> (estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL})</span></span>
		{:else}
			<span class="text-pqs-bluegray" title="Not yet benchmarked">—<span class="sr-only"> not yet benchmarked</span></span>
		{/if}
	</td>
{/snippet}

<div class="overflow-hidden rounded border border-pqs-ashgray shadow-sm dark:border-pqs-steel">
	<div class="overflow-x-auto">
		<table class="min-w-full text-sm">
			<thead class="sticky top-0 z-10">
				<tr class="bg-pqs-steel font-heading text-xs text-white">
					{#each COLUMNS as col, i (col.key)}
						{#if i === 2}
							<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-left font-semibold">Programmes</th>
						{/if}
						<th scope="col" aria-sort={ariaSort(col.key)} class="whitespace-nowrap p-0">
							<button
								type="button"
								onclick={() => setSort(col.key)}
								class="w-full cursor-pointer select-none px-3 py-2.5 font-semibold hover:bg-pqs-steel-light {col.numeric ? 'text-right' : 'text-left'}"
							>
								{col.label}{sortIndicator(col.key)}
							</button>
						</th>
					{/each}
					<th scope="col" class="px-3 py-2.5 text-left text-xs font-semibold">Assumption</th>
				</tr>
			</thead>
			<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
				{#each sortedRows as row, i (rowKey(row))}
					{@const key = rowKey(row)}
					{@const badges = securityBadges(row)}
					{@const openBadge = badges.find((b) => b.kind === open[key])}
					{@const detailId = `${idPrefix}-detail-${i}`}
					<tr class="hover:bg-pqs-smoke dark:hover:bg-pqs-steel/30">
						<td class="whitespace-nowrap px-3 py-1.5">
							<a
								href={row.website}
								target="_blank"
								rel="noopener noreferrer"
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
						<td class="whitespace-nowrap px-3 py-1.5 text-pqs-steel dark:text-pqs-bluegray">{row.category}</td>
						<td class="px-3 py-1.5"><ProgrammeChips chips={programmeChips(row, allProgrammes)} /></td>
						<td class="whitespace-nowrap px-3 py-1.5 font-mono text-xs" title={row.parameterset}>{shortParam(row)}</td>
						<td class="px-3 py-1.5 text-right tabular-nums text-pqs-steel dark:text-pqs-bluegray">
							{#if row.level !== null}
								{row.level === 'Pre-Quantum' ? 'N/A' : row.level}
							{:else if row.ngccLevel !== null}
								<span class="font-semibold text-[#6b21a8] dark:text-[#d8b4fe]" title="China NGCC target in classical bits; not a NIST level">{row.ngccLevel}-bit</span>
							{:else}
								—
							{/if}
						</td>
						<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pk)}</td>
						<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.ct)}</td>
						<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pkPlusCt)}</td>
						{@render timeCell(row.keygenUs, row.keygenCycles)}
						{@render timeCell(row.encapsUs, row.encapsCycles)}
						{@render timeCell(row.decapsUs, row.decapsCycles)}
						<td class="px-3 py-1.5 text-pqs-steel/80 dark:text-pqs-bluegray">{row.assumption}</td>
					</tr>
					{#if badges.length > 0}
						<tr id={detailId} hidden={!openBadge} class={openBadge ? DETAIL_TONE[openBadge.kind] : ''}>
							<td colspan={COLSPAN} class="whitespace-normal px-3 py-2.5 pl-9 text-sm leading-relaxed text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-apricot [&_a]:underline">
								{#if openBadge}
									<strong class="font-heading">{openBadge.title}:</strong>
									{@html openBadge.html}
									{#if openBadge.href && /^https?:\/\//.test(openBadge.href)}
										<a href={openBadge.href} target="_blank" rel="noopener noreferrer">Full findings report</a>
									{/if}
								{/if}
							</td>
						</tr>
					{/if}
				{/each}
				{#if sortedRows.length === 0}
					<tr>
						<td colspan={COLSPAN} class="px-3 py-10 text-center text-pqs-bluegray">No KEMs to display.</td>
					</tr>
				{/if}
			</tbody>
		</table>
	</div>
	<div class="border-t border-pqs-ashgray bg-pqs-smoke px-3 py-1.5 font-heading text-xs text-pqs-steel dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray">
		<span>{sortedRows.length} parameter set{sortedRows.length === 1 ? '' : 's'}</span>{#if recommendedOnly}{' '}<span>shown (recommended sets)</span>{/if}
	</div>
	<TableLegend />
</div>
