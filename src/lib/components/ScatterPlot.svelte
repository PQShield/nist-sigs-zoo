<script lang="ts">
	import * as Plot from '@observablehq/plot';
	import { onMount, untrack } from 'svelte';
	import { dotColor, dotSymbol, dotTitle } from '$lib/plotHelpers';
	import { getFilterStore } from '$lib/filterStore';
	import { themeStore } from '$lib/themeStore';
	import { zoom as d3zoom, zoomIdentity } from 'd3-zoom';
	import type { ZoomBehavior, ZoomTransform } from 'd3-zoom';
	import { select } from 'd3-selection';
	import { scaleLog } from 'd3-scale';
	import { extent } from 'd3-array';

	const { filteredRows } = getFilterStore();

	let container = $state<HTMLDivElement | null>(null);
	let width = $state(600);
	let isZoomed = $state(false);

	// Plain vars — zoom state lives outside Svelte reactivity to avoid effect tracking loops
	let xDomain: [number, number] | null = null;
	let yDomain: [number, number] | null = null;
	let currentTransform: ZoomTransform = zoomIdentity;
	let zoomBehavior: ZoomBehavior<SVGSVGElement, unknown> | null = null;

	// Reference dimensions for zoom math — decoupled from Plot's pixel layout
	const REF_W = 1000;
	const REF_H = 600;

	function buildZoom(data: typeof $filteredRows) {
		const pks = data.map((d) => d.pk).filter((v) => v > 0);
		const sigs = data.map((d) => d.sig).filter((v) => v > 0);
		if (!pks.length || !sigs.length) return;

		const [pkMin, pkMax] = extent(pks) as [number, number];
		const [sigMin, sigMax] = extent(sigs) as [number, number];

		const xScale = scaleLog()
			.domain([pkMin * 0.8, pkMax * 1.25])
			.range([0, REF_W]);
		const yScale = scaleLog()
			.domain([sigMin * 0.8, sigMax * 1.25])
			.range([REF_H, 0]);

		zoomBehavior = d3zoom<SVGSVGElement, unknown>()
			.scaleExtent([0.5, 100])
			.extent([
				[0, 0],
				[REF_W, REF_H]
			])
			.on('zoom', ({ transform }) => {
				currentTransform = transform;
				const nx = transform.rescaleX(xScale).domain() as [number, number];
				const ny = transform.rescaleY(yScale).domain() as [number, number];
				if (nx[0] > 0 && ny[0] > 0) {
					xDomain = nx;
					yDomain = ny;
					isZoomed = transform.k !== 1 || transform.x !== 0 || transform.y !== 0;
					render();
				}
			});
	}

	function attachZoom() {
		const svg = container?.querySelector('svg');
		if (svg && zoomBehavior) {
			select(svg as SVGSVGElement)
				.call(zoomBehavior)
				.call(zoomBehavior.transform, currentTransform);
		}
	}

	function render() {
		if (!container) return;
		const data = $filteredRows;
		const isDark = document.documentElement.classList.contains('dark');
		const color = (d: (typeof data)[0]) => dotColor(d, isDark);

		container.replaceChildren(
			Plot.plot({
				width,
				height: Math.min(width * 0.6, 500),
				x: {
					type: 'log',
					label: 'Public key size (bytes)',
					grid: true,
					...(xDomain ? { domain: xDomain } : {})
				},
				y: {
					type: 'log',
					label: 'Signature size (bytes)',
					grid: true,
					...(yDomain ? { domain: yDomain } : {})
				},
				style: {
					background: 'transparent',
					color: isDark ? '#C5CADA' : '#061128'
				},
				marks: [
					Plot.dot(data, {
						x: 'pk',
						y: 'sig',
						tip: true,
						title: dotTitle,
						stroke: color,
						symbol: dotSymbol,
						fill: color,
						fillOpacity: 0.8
					}),
					Plot.crosshair(data, { x: 'pk', y: 'sig' })
				]
			})
		);

		attachZoom();
	}

	function resetZoom() {
		xDomain = null;
		yDomain = null;
		isZoomed = false;
		currentTransform = zoomIdentity;
		render();
	}

	onMount(() => {
		const ro = new ResizeObserver((entries) => {
			width = entries[0].contentRect.width || 600;
		});
		if (container) ro.observe(container);
		buildZoom($filteredRows);
		render();
		return () => ro.disconnect();
	});

	// Data or theme change: reset zoom and re-render
	$effect(() => {
		const data = $filteredRows;
		$themeStore;
		untrack(() => {
			xDomain = null;
			yDomain = null;
			isZoomed = false;
			currentTransform = zoomIdentity;
			buildZoom(data);
			render();
		});
	});

	// Resize: re-render preserving zoom
	$effect(() => {
		width;
		untrack(() => render());
	});
</script>

<div class="relative w-full">
	{#if isZoomed}
		<button
			onclick={resetZoom}
			class="absolute top-2 right-2 z-10 cursor-pointer rounded px-2 py-1 text-xs bg-gray-100 dark:bg-gray-700 hover:bg-gray-200 dark:hover:bg-gray-600"
		>
			Reset zoom
		</button>
	{/if}
	<div bind:this={container} class="w-full [&_svg]:overflow-visible"></div>
</div>
