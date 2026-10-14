<script lang="ts">
	import { goto } from '$app/navigation';
	import { base } from '$app/paths';
	import { attachChartNav } from '$lib/chartNav';
	import { slugOf } from '$lib/schemeIndex';
	import { schemeHref } from '$lib/slugs';
	import { onMount } from 'svelte';
	import { getFilterStore } from '$lib/filterStore';
	import { themeStore } from '$lib/themeStore';
	import { stripHtml } from '$lib/html';
	import {
		chartLegend,
		familyKey,
		FAMILY_SHAPES,
		levelColourScale,
		levelDescription,
		levelKey,
		RINGS,
		securityStatus,
		STANDARDISED,
		type LegendGroup
	} from '$lib/chartLegend';
	import { partitionPlottable, sigAxisValue, timeTooltip } from '$lib/chartData';
	import { CPU_GHZ_LABEL } from '$lib/constants';
	import type { AxisField, ScaleType } from '$lib/types';
	import ChartLegend from './ChartLegend.svelte';
	import OmittedNote from './OmittedNote.svelte';

	let {
		xField = 'pk' as AxisField,
		yField = 'sig' as AxisField,
		xScale = 'log' as ScaleType,
		yScale = 'log' as ScaleType,
	}: { xField?: AxisField; yField?: AxisField; xScale?: ScaleType; yScale?: ScaleType } = $props();

	const AXIS_TITLES: Record<AxisField, string> = {
		pk: 'Public key (bytes)',
		sig: 'Signature (bytes)',
		pkPlusSig: 'pk + sig (bytes)',
		signingCycles: 'Signing (cycles)',
		verificationCycles: 'Verification (cycles)',
		signingUs: 'Signing (µs)',
		verificationUs: 'Verification (µs)',
		signingTime: 'Signing time (µs)',
		verificationTime: 'Verification time (µs)',
	};

	const { filteredRows } = getFilterStore();

	let container = $state<HTMLDivElement | null>(null);
	let viewHandle: { finalize(): void } | null = null;
	let renderSeq = 0;
	let alive = true;
	let detachNav: (() => void) | null = null;
	let renderQueue: Promise<void> = Promise.resolve();
	let renderError = $state(false);
	// Wide screens put the legend beside the plot; when the legend is taller, the plot grows to match.
	const MIN_PLOT_HEIGHT = 350;
	let plotHeight = $state(MIN_PLOT_HEIGHT);
	let chartExtra = $state(0); // rendered height beyond the plot area (axes, titles), measured per render
	let legendHeight = $state(0);
	let belowHeight = $state(0);
	let wide = $state(false);
	$effect(() => {
		const mq = window.matchMedia('(min-width: 1024px)');
		const on = () => (wide = mq.matches);
		on();
		mq.addEventListener('change', on);
		return () => mq.removeEventListener('change', on);
	});
	$effect(() => {
		const target = wide
			? Math.max(MIN_PLOT_HEIGHT, Math.round(legendHeight - belowHeight - chartExtra))
			: MIN_PLOT_HEIGHT;
		if (Math.abs(target - plotHeight) > 4) plotHeight = target;
	});
	let legend = $state<LegendGroup[]>([]);
	let omitted = $state(0);

	const TIME_FIELDS = new Set<AxisField>(['signingTime', 'verificationTime']);
	const timeAxes = $derived(TIME_FIELDS.has(xField) || TIME_FIELDS.has(yField));
	const TIME_FOOTNOTE = `Timings in µs. Where only cycle counts exist, time is extrapolated at ${CPU_GHZ_LABEL}.`;

	// Serialize renders: concurrent vega-embed calls on the same container can
	// leave a stale, finalized chart in the DOM when they resolve out of order.
	function render() {
		const seq = ++renderSeq;
		renderQueue = renderQueue.then(async () => {
			try {
				await doRender(seq);
				renderError = false;
			} catch (err) {
				console.error('scatter plot render failed', err);
				renderError = true;
			}
		});
	}

	async function doRender(seq: number) {
		if (!alive || !container || seq !== renderSeq) return;

		const { default: embed } = await import('vega-embed');
		if (!alive || seq !== renderSeq) return;
		const { plotted: data, omitted: missing } = partitionPlottable($filteredRows, sigAxisValue, xField, yField);
		omitted = missing.length;
		const isDark = document.documentElement.classList.contains('dark');

		const values = data.map((d) => {
			const notes = [
				d.broken && !d.classical ? `⚠ ${stripHtml(d.broken)}` : null,
				d.warning ? `⚠ ${stripHtml(d.warning)}` : null
			]
				.filter(Boolean)
				.join(' | ');

			const tooltip: Record<string, string | number> = {
				Scheme: d.scheme,
				'Parameter set': d.parameterset,
				...(d.version ? { Version: d.version } : {}),
				Family: d.category,
				'Security level': levelDescription(d.level, d.ngccLevel),
				'pk (bytes)': d.pk.toLocaleString(),
				'sig (bytes)': d.sig.toLocaleString(),
				'pk+sig (bytes)': d.pkPlusSig.toLocaleString(),
				...(d.signingCycles > 0 ? { 'Signing (cycles)': d.signingCycles.toLocaleString() } : {}),
				...(d.verificationCycles > 0 ? { 'Verification (cycles)': d.verificationCycles.toLocaleString() } : {}),
				...timeTooltip([
					{ label: 'Signing', us: d.signingUs, cycles: d.signingCycles },
					{ label: 'Verification', us: d.verificationUs, cycles: d.verificationCycles }
				]),
				...(notes ? { Notes: notes } : {}),
				'Scheme page': 'click the point'
			};

			return {
				scheme: d.scheme,
				href: schemeHref(base, slugOf(d.scheme)),
				parameterset: d.parameterset,
				pk: d.pk,
				sig: d.sig,
				pkPlusSig: d.pkPlusSig,
				signingCycles: d.signingCycles > 0 ? d.signingCycles : null,
				verificationCycles: d.verificationCycles > 0 ? d.verificationCycles : null,
				signingUs: d.signingUs,
				verificationUs: d.verificationUs,
				signingTime: sigAxisValue(d, 'signingTime'),
				verificationTime: sigAxisValue(d, 'verificationTime'),
				family: familyKey(d),
				level: levelKey(d.level, d.ngccLevel),
				security: securityStatus(d),
				tooltip
			};
		});

		const colour = levelColourScale(values.map((v) => v.level), isDark);
		const families = Object.keys(FAMILY_SHAPES).filter((f) => values.some((v) => v.family === f));
		legend = chartLegend(values, isDark);
		const tone = isDark ? 'dark' : 'light';

		// eslint-disable-next-line @typescript-eslint/no-explicit-any
		const spec: any = {
			$schema: 'https://vega.github.io/schema/vega-lite/v6.json',
			width: 'container',
			height: plotHeight,
			autosize: { type: 'fit-x', resize: true },
			data: { values },
			resolve: { scale: { x: 'shared', y: 'shared', color: 'independent', shape: 'independent' } },
			layer: [
				{
					params: [
						{
							name: 'grid',
							select: { type: 'interval', encodings: ['x', 'y'] },
							bind: 'scales'
						}
					],
					mark: { type: 'point', filled: true, size: 100, cursor: 'pointer' },
					encoding: {
						size: {
							condition: [
								{ test: `datum.family === '${STANDARDISED}'`, value: 180 },
								{ test: "datum.family === 'Lattices'", value: 70 }
							],
							value: 100
						},
						color: { field: 'level', type: 'nominal', scale: colour, legend: null },
						shape: {
							field: 'family',
							type: 'nominal',
							scale: { domain: families, range: families.map((f) => FAMILY_SHAPES[f]) },
							legend: null
						}
					}
				},
				{
					transform: [{ filter: "datum.security === 'broken'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 120, strokeWidth: 2, cursor: 'pointer' },
					encoding: { color: { value: RINGS.broken[tone] } }
				},
				{
					transform: [{ filter: "datum.security === 'warning'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 120, strokeWidth: 2, cursor: 'pointer' },
					encoding: { color: { value: RINGS.warning[tone] } }
				}
			],
			encoding: {
				x: {
					field: xField,
					type: 'quantitative',
					scale: { type: xScale, ...(xScale === 'log' ? { padding: 14 } : {}) },
					axis: { title: AXIS_TITLES[xField], grid: true, format: 's' }
				},
				y: {
					field: yField,
					type: 'quantitative',
					scale: { type: yScale, ...(yScale === 'log' ? { padding: 14 } : {}) },
					axis: { title: AXIS_TITLES[yField], grid: true, format: 's' }
				},
				tooltip: { field: 'tooltip', type: 'nominal' }
			},
			config: {
				background: 'transparent',
				axis: {
					gridColor: isDark ? '#334155' : '#E2E8F0',
					tickColor: isDark ? '#475569' : '#94A3B8',
					labelColor: isDark ? '#94A3B8' : '#475569',
					titleColor: isDark ? '#C5CADA' : '#061128',
					domainColor: isDark ? '#475569' : '#94A3B8'
				},
				view: { stroke: 'transparent' }
			}
		};

		detachNav?.();
		viewHandle?.finalize();
		viewHandle = null;
		const result = await embed(container, spec, { actions: false, renderer: 'svg' });
		if (!alive) {
			result.finalize();
			return;
		}
		viewHandle = result;
		if (container) chartExtra = container.clientHeight - plotHeight;
		detachNav = attachChartNav(container, result.view, goto);
	}

	onMount(() => {
		return () => {
			alive = false;
			detachNav?.();
			viewHandle?.finalize();
		};
	});

	$effect(() => {
		$filteredRows;
		plotHeight;
		$themeStore;
		xField; yField; xScale; yScale;
		render();
	});
</script>

<div class="flex flex-col gap-4 lg:flex-row lg:items-start">
	<div class="min-w-0 flex-1">
		<div bind:this={container} class="w-full"></div>
		<div bind:clientHeight={belowHeight}>
		{#if renderError}
			<p class="mt-1 text-xs text-pqs-scarlet" role="alert">Chart failed to render.</p>
		{/if}
		<OmittedNote count={omitted} />
		<div class="mt-1 flex items-center justify-between">
			<p class="text-xs text-pqs-steel dark:text-pqs-bluegray">Scroll to zoom · drag to pan<span class="sm:hidden"> · tap a point for details</span></p>
			<button
				type="button"
				onclick={render}
				class="text-xs text-pqs-steel hover:text-pqs-apricot dark:text-pqs-bluegray dark:hover:text-pqs-apricot transition-colors"
			>Reset zoom</button>
		</div>
		</div>
	</div>
	<div bind:clientHeight={legendHeight} class="lg:shrink-0">
		<ChartLegend groups={legend} footnote={timeAxes ? TIME_FOOTNOTE : undefined} />
	</div>
</div>
