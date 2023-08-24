$(document).foundation();

const CPUSPEED = 2_500_000_000;

const schemes = await d3.csv("data/schemes.csv", (d) => {
  return {
    Scheme: d.Scheme,
    Status: d["NIST status"],
    Website: d.Website,
    Category: d.Category,
    Broken: d.Broken === "no" ? false : d.Broken,
    Classical: d.Broken === "classical",
    Assumption: d.Assumption,
  };
});
const properties = await d3.csv("data/parametersets.csv", (d) => {
  let signcycles;
  let verifycycles;
  let extrapolated;
  if (parseInt(d["signing (cycles)"].replace(/,/g, "")) > 0) {
    extrapolated = false;
    signcycles = parseInt(d["signing (cycles)"].replace(/,/g, ""));
    verifycycles = parseInt(d["verification (cycles)"].replace(/,/g, ""));
  } else {
    extrapolated = true;
    signcycles =
      (CPUSPEED * parseFloat(d["signing (ms)"].replace(/,/g, ""))) / 1000;
    verifycycles =
      (CPUSPEED * parseFloat(d["verification (ms)"].replace(/,/g, ""))) / 1000;
  }

  let broken = schemes.find((s) => s.Scheme == d.Scheme && s.Broken);
  if (broken === undefined) {
    broken = false;
  } else {
    broken = broken.Broken;
  }
  const classical = schemes.some((s) => s.Scheme == d.Scheme && s.Classical);

  const level = d["Security level"] === "Pre-Quantum" ? "Pre-Quantum" : +d["Security level"];

  return {
    Scheme: d.Scheme,
    Parameterset: d.Parameterset,
    Level: level,
    Pk: +d["pk size"].replace(/,/g, ""),
    Sig: +d["sig size"].replace(/,/g, ""),
    PkPlusSig:
      parseFloat(d["pk size"].replace(/,/g, "")) +
      parseInt(d["sig size"].replace(/,/g, "")),
    SigningCycles: signcycles,
    VerificationCycles: verifycycles,
    SigningTime: parseFloat(d["signing (ms)"].replace(/,/g, "")),
    VerificationTime: parseFloat(d["verification (ms)"].replace(/,/g, "")),
    Extrapolated: extrapolated,
    Broken: broken,
    Classical: classical
  };
});

const categories = new Set(schemes.map((s) => s.Category));

const table = d3.select("#scheme-table");
const propsTable = d3.select("#properties-table");

function cleanId(name) {
  return name.replace(/[^a-zA-Z0-9]/g, "_");
}

function reenableCategoryForScheme(event, scheme) {
  if (!event.target.checked) {
    return;
  }
  schemes.forEach((s) => {
    console.log(s, scheme);
    if (s.Scheme === scheme) {
      d3.select("#switch-" + cleanId(s.Category)).property("checked", true);
      return;
    }
  });
}

let schemeSortingDirection = 1;
let nowSortingScheme = "Scheme";
let propertiesSortingDirection = 1;
let nowSortingProperties = "Scheme";
let performanceSortingDirection = 1;
let nowSortingPerformance = "Scheme";

function sortAndFilterProperties() {
  let selectedPropsSchemes = schemes.map((d) => d.Scheme);
  selectedPropsSchemes = d3
    .selectAll("#props-schemes-filter input:checked")
    .data();

  const minPk = parseInt(d3.select("#props-min-pk").property("value"));
  const maxPk = parseInt(d3.select("#props-max-pk").property("value"));
  const minSig = parseInt(d3.select("#props-min-sig").property("value"));
  const maxSig = parseInt(d3.select("#props-max-sig").property("value"));
  const minPkPlusSig = parseInt(
    parseInt(d3.select("#props-min-pkplussig").property("value"))
  );
  const maxPkPlusSig = parseInt(
    parseInt(d3.select("#props-max-pkplussig").property("value"))
  );

  const minSignCycles = parseInt(d3.select("#perf-min-sign").property("value"));
  const maxSignCycles = parseInt(d3.select("#perf-max-sign").property("value"));
  const minVerifyCycles = parseInt(d3.select("#perf-min-verify").property("value"));
  const maxVerifyCycles = parseInt(d3.select("#perf-max-verify").property("value"));

  const applyCycles = d3.select("#props-sync-filters").property("checked");

  const selectedPropsLevels = d3
    .selectAll("#props-levels-filter input:checked")
    .data();
  return properties
    .filter(
      (p) =>
        selectedPropsSchemes.includes(p.Scheme) &&
        selectedPropsLevels.includes(p.Level) &&
        p.Pk >= minPk &&
        maxPk >= p.Pk &&
        p.Sig >= minSig &&
        maxSig >= p.Sig &&
        p.PkPlusSig >= minPkPlusSig &&
        maxPkPlusSig >= p.PkPlusSig &&
        (!applyCycles ||
          (p.SigningCycles >= minSignCycles &&
            p.SigningCycles <= maxSignCycles &&
            p.VerificationCycles >= minVerifyCycles &&
            p.VerificationCycles <= maxVerifyCycles))
    )
    .sort(
      (a, b) =>
        propertiesSortingDirection *
        d3.ascending(a[nowSortingProperties], b[nowSortingProperties])
    );
}

function updateTable(event) {
  console.log("updating tables");
  const selectedCategories = d3.selectAll(".category > input:checked").data();

  function sortAndFilterSchemes() {
    return schemes
      .filter((s) => selectedCategories.includes(s.Category))
      .sort(
        (a, b) =>
          schemeSortingDirection *
          d3.ascending(a[nowSortingScheme], b[nowSortingScheme])
      );
  }

  // data
  table
    .select("tbody")
    .selectAll("tr")
    .data(sortAndFilterSchemes(), (d) => d.Scheme)
    .join((enter) =>
      enter.append((d) => {
        const row = d3.create("tr").property("data-scheme", d.Scheme);
        const cell = row.append("td");
        cell
          .append("a")
          .attr("href", d.Website)
          .attr("target", "_blank")
          .text(d.Scheme);
        if (d.Classical) {
          cell
            .append("span")
            .property("title", "This scheme is not resistant to quantum computers")
            .text(" ⚠️");
        } else if (d.Broken) {
          cell
            .append("span")
            .property("title", "This submission has security vulnerabilities: " + d.Broken)
            .text(" ⚠️");
        }
        row.append("td").text(d.Status);
        row.append("td").text(d.Category);
        row.append("td").text(d.Assumption);

        return row.node();
      })
    );

  d3.select("#properties-table")
    .select("tbody")
    .selectAll("tr")
    .data(sortAndFilterProperties(), (d) => d.Scheme + d.Parameterset)
    .join((enter) =>
      enter.append((d) => {
        const row = d3.create("tr");

        const cell = row.append("td").text(d.Scheme);
        if (d.Broken) {
          cell
            .append("span")
            .property("title", "This submission has security vulnerabilities: " + d.Broken)
            .text(" ⚠️");
        }
        row.append("td").text(d.Parameterset);
        if (d.Classical) {
          row.append("td").text("Pre-Q");
        } else {
          row.append("td").text(d.Level);
        }
        row
          .append("td")
          .text(d.Pk.toLocaleString())
          .attr("style", "text-align: right");
        row
          .append("td")
          .text(d.Sig.toLocaleString())
          .attr("style", "text-align: right");
        row
          .append("td")
          .text(d.PkPlusSig.toLocaleString())
          .attr("style", "text-align: right");
        return row.node();
      })
    );

  let selectedPerfSchemes = schemes.map((d) => d.Scheme);
  if (event !== undefined) {
    selectedPerfSchemes = d3
      .selectAll("#perf-schemes-filter input:checked")
      .data();
  }

  const selectedPerfLevels = d3
    .selectAll("#perf-levels-filter input:checked")
    .data();

  function sortAndFilterPerformance() {
    const minSignCycles = parseInt(d3.select("#perf-min-sign").property("value"));
    const maxSignCycles = parseInt(d3.select("#perf-max-sign").property("value"));
    const minVerifyCycles = parseInt(d3.select("#perf-min-verify").property("value"));
    const maxVerifyCycles = parseInt(d3.select("#perf-max-verify").property("value"));

    const applySizes = d3.select("#perf-sync-filters").property("checked");
    const minPk = parseInt(d3.select("#props-min-pk").property("value"));
    const maxPk = parseInt(d3.select("#props-max-pk").property("value"));
    const minSig = parseInt(d3.select("#props-min-sig").property("value"));
    const maxSig = parseInt(d3.select("#props-max-sig").property("value"));
    const minPkPlusSig = parseInt(
      d3.select("#props-min-pkplussig").property("value")
    );
    const maxPkPlusSig = parseInt(
      d3.select("#props-max-pkplussig").property("value")
    );

    return properties
      .filter(
        (p) =>
          selectedPerfSchemes.includes(p.Scheme) &&
          selectedPerfLevels.includes(p.Level) &&
          p.SigningCycles >= minSignCycles &&
          p.SigningCycles <= maxSignCycles &&
          p.VerificationCycles >= minVerifyCycles &&
          p.VerificationCycles <= maxVerifyCycles &&
          (!applySizes ||
            (p.Pk >= minPk &&
              maxPk >= p.Pk &&
              p.Sig >= minSig &&
              maxSig >= p.Sig &&
              p.PkPlusSig >= minPkPlusSig &&
              maxPkPlusSig >= p.PkPlusSig))
      )
      .sort(
        (a, b) =>
          performanceSortingDirection *
          d3.ascending(a[nowSortingPerformance], b[nowSortingPerformance])
      );
  }

  d3.select("#performance-table")
    .select("tbody")
    .selectAll("tr")
    .data(sortAndFilterPerformance(), (d) => d.Scheme + d.Parameterset)
    .join((enter) =>
      enter.append((d) => {
        const row = d3.create("tr");

        const cell = row.append("td").text(d.Scheme);
        if (d.Classical) {
          cell
            .append("span")
            .property("title", "This scheme is not resistant to quantum computers")
            .text(" ⚠️");
        } else if (d.Broken) {
          cell
            .append("span")
            .property("title", "This submission has security vulnerabilities: " + d.Broken)
            .text(" ⚠️");
        }
        row.append("td").text(d.Parameterset);
        if (d.Classical) {
          row.append("td").text("Pre-Q");
        } else {
          row.append("td").text(d.Level);
        }
        let extrapolated_text_sign;
        let extrapolated_text_verify;
        if (d.Extrapolated) {
          extrapolated_text_sign =
            "Reported as " + d.SigningTime.toLocaleString() + " ms.";
          extrapolated_text_verify =
            "Reported as " + d.VerificationTime.toLocaleString() + " ms.";
        }
        row
          .append("td")
          .text(d.SigningCycles.toLocaleString())
          .classed("extrapolated", d.Extrapolated)
          .property("title", extrapolated_text_sign)
          .attr("style", "text-align: right");
        row
          .append("td")
          .text(d.VerificationCycles.toLocaleString())
          .classed("extrapolated", d.Extrapolated)
          .property("title", extrapolated_text_verify)
          .attr("style", "text-align: right");
        return row.node();
      })
    );

  updatePlot();
}

function switchSchemesForCategory(event, category) {
  const enabledCategory = event.target.checked;
  schemes.forEach((scheme) => {
    console.log(scheme, event,category);
    if (scheme.Category === category) {
      d3.select("#props-switch-" + cleanId(scheme.Scheme)).property("checked", enabledCategory);
      d3.select("#perf-switch-" + cleanId(scheme.Scheme)).property("checked", enabledCategory);
    }
  });
}

d3.select("#categories")
  .selectAll("div")
  .classed("grid-x", true)
  .data([...categories].sort(d3.ascending))
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true)
      .classed("category", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .attr("id", "switch-" + cleanId(d))
      .classed("switch-input categories-filter", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", function(event) {
        switchSchemesForCategory(event, d);
        updateTable(event);
      });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "switch-" + cleanId(d))
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide " + d)
          .node()
      );
    cat.append("span").classed("cell auto", true).text(d);

    return cat.node();
  });

d3.select("#props-schemes-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data(schemes.map((s) => s.Scheme).sort(d3.ascending))
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("scheme-filter", true)
      .attr("id", "props-switch-" + cleanId(d))
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        d3.select("#perf-switch-" + cleanId(d)).property(
          "checked",
          e.target.checked
        );
        reenableCategoryForScheme(e, d);
        updateTable(e);
      });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "props-switch-" + cleanId(d))
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide " + d)
          .node()
      );
    cat.append("span").classed("cell auto", true).text(d);

    return cat.node();
  });

d3.select("#props-levels-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data(["Pre-Quantum", 1, 2, 3, 4, 5])
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("nistlevel-filter", true)
      .attr("id", "props-switch-level-" + d)
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        updateTable(e);
      });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "props-switch-level-" + d)
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide NIST level " + d)
          .node()
      );
    cat
      .append("span")
      .classed("cell auto", true)
      .text(d == "Pre-Quantum" ? "Pre-Quantum" : "Level " + d);

    return cat.node();
  });

d3.select("#perf-levels-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data(["Pre-Quantum", 1, 2, 3, 4, 5])
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("nistlevel-filter", true)
      .attr("id", "perf-switch-level-" + d)
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        updateTable(e);
      });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "perf-switch-level-" + d)
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide NIST level " + d)
          .node()
      );
    cat
      .append("span")
      .classed("cell auto", true)
      .text(d == "Pre-Quantum" ? "Pre-Quantum" : "Level " + d)

    return cat.node();
  });

d3.select("#props-min-pk")
  .property(
    "value",
    d3.min(properties, (d) => d.Pk)
  )
  .on("change", updateTable);
d3.select("#props-max-pk")
  .property(
    "value",
    d3.max(properties, (d) => d.Pk)
  )
  .on("change", updateTable);
d3.select("#props-min-sig")
  .property(
    "value",
    d3.min(properties, (d) => d.Sig)
  )
  .on("change", updateTable);
d3.select("#props-max-sig")
  .property(
    "value",
    d3.max(properties, (d) => d.Sig)
  )
  .on("change", updateTable);
d3.select("#props-min-pkplussig")
  .property(
    "value",
    d3.min(properties, (d) => d.PkPlusSig)
  )
  .on("change", updateTable);
d3.select("#props-max-pkplussig")
  .property(
    "value",
    d3.max(properties, (d) => d.PkPlusSig)
  )
  .on("change", updateTable);

d3.select("#perf-min-sign")
  .property(
    "value",
    d3.min(properties, (d) => d.SigningCycles)
  )
  .on("change", updateTable);
d3.select("#perf-max-sign")
  .property(
    "value",
    d3.max(properties, (d) => d.SigningCycles)
  )
  .on("change", updateTable);
d3.select("#perf-min-verify")
  .property(
    "value",
    d3.min(properties, (d) => d.VerificationCycles)
  )
  .on("change", updateTable);
d3.select("#perf-max-verify")
  .property(
    "value",
    d3.max(properties, (d) => d.VerificationCycles)
  )
  .on("change", updateTable);

d3.select("#perf-schemes-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data(schemes.map((s) => s.Scheme).sort(d3.ascending))
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("scheme-filter", true)
      .attr("id", "perf-switch-" + cleanId(d))
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        d3.select("#props-switch-" + cleanId(d)).property(
          "checked",
          e.target.checked
        );
        reenableCategoryForScheme(e, d);
        updateTable(e);
      });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "perf-switch-" + cleanId(d))
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide " + d)
          .node()
      );
    cat.append("span").classed("cell auto", true).text(d);

    return cat.node();
  });

d3.selectAll(".select-all-params").on("click", (e) => {
  d3.selectAll(".scheme-filter").property("checked", true);
  d3.selectAll(".categories-filter").property("checked", true);
  updateTable(e);
});
d3.selectAll(".select-none-params").on("click", (e) => {
  d3.selectAll(".scheme-filter").property("checked", false);
  updateTable(e);
});

updateTable();

function handleSortingSchemes(what) {
  return (e) => {
    if (nowSortingScheme === what) {
      schemeSortingDirection *= -1;
    } else {
      schemeSortingDirection = 1;
      nowSortingScheme = what;
    }
    updateTable(e);
  };
}

function handleSortingProperties(what) {
  return (e) => {
    if (nowSortingProperties === what) {
      propertiesSortingDirection *= -1;
    } else {
      propertiesSortingDirection = 1;
      nowSortingProperties = what;
    }
    updateTable(e);
  };
}

function handleSortingPerformance(what) {
  return (e) => {
    if (nowSortingPerformance === what) {
      performanceSortingDirection *= -1;
    } else {
      performanceSortingDirection = 1;
      nowSortingPerformance = what;
    }
    updateTable(e);
  };
}

d3.select("#header-schemes-scheme").on("click", handleSortingSchemes("Scheme"));
d3.select("#header-schemes-status").on("click", handleSortingSchemes("Status"));
d3.select("#header-schemes-category").on(
  "click",
  handleSortingSchemes("Category")
);
d3.select("#header-schemes-assumption").on(
  "click",
  handleSortingSchemes("Assumption")
);

d3.select("#header-properties-scheme").on(
  "click",
  handleSortingProperties("Scheme")
);
d3.select("#header-properties-parameterset").on(
  "click",
  handleSortingProperties("Parameterset")
);
d3.select("#header-properties-level").on(
  "click",
  handleSortingProperties("Level")
);
d3.select("#header-properties-pk").on("click", handleSortingProperties("Pk"));
d3.select("#header-properties-sig").on("click", handleSortingProperties("Sig"));
d3.select("#header-properties-pksig").on(
  "click",
  handleSortingProperties("PkPlusSig")
);

d3.select("#header-performance-scheme").on(
  "click",
  handleSortingPerformance("Scheme")
);
d3.select("#header-performance-parameterset").on(
  "click",
  handleSortingPerformance("Parameterset")
);
d3.select("#header-performance-level").on(
  "click",
  handleSortingPerformance("Level")
);
d3.select("#header-performance-sign").on(
  "click",
  handleSortingPerformance("SigningCycles")
);
d3.select("#header-performance-verify").on(
  "click",
  handleSortingPerformance("VerificationCycles")
);

d3.select("#perf-sync-filters").on("change", updateTable);
d3.select("#props-sync-filters").on("change", updateTable);

// function getKeySizeChart() {
//   // Declare the chart dimensions and margins.
//   const width = 600;
//   const height = 200;
//   const margin = { top: 20, right: 0, bottom: 30, left: 50 };

//   console.log(properties)

//   function zoom(svg) {
//     const extent = [
//       [margin.left, margin.top],
//       [width - margin.right, height - margin.top],
//     ];

//     svg.call(
//       d3
//         .zoom()
//         .scaleExtent([1, 8])
//         .translateExtent(extent)
//         .extent(extent)
//         .on("zoom", zoomed)
//     );

//     function zoomed(event) {
//       x.range(
//         [margin.left, width - margin.right].map((d) =>
//           event.transform.applyX(d)
//         )
//       );
//       svg.selectAll(".bars rect").attr("x", d => d.Scheme + " " + d.Parameterset).attr("width", x.bandwidth());
//       svg.selectAll(".x-axis").call(xAxis);
//     }
//   }

//   const x = d3
//     .scaleBand(properties.map((d) => d.Scheme + " " + d.Parameterset))
//     .range([margin.left, width - margin.right])
//     .padding(0.1);

//   const y = d3
//     .scaleLinear()
//     .domain([1, d3.max(properties, (d) => d.Pk)])
//     .nice()
//     .range([height - margin.bottom, margin.top]);

//   const xAxis = (g) =>
//     g
//       .attr("transform", `translate(0, ${height - margin.bottom})`)
//       .call(d3.axisBottom(x).tickSizeOuter(0));

//   const yAxis = (g) =>
//     g
//       .attr("transform", `translate(${margin.left},0)`)
//       .call(d3.axisLeft(y))
//       .call((g) => g.select(".domain").remove());

//   const svg = d3
//       .create("svg")
//       .attr("viewBox", [0, 0, width, height]);
//   svg
//     .append("g")
//     .attr("class", "bars")
//     .attr("fill", "steelblue")
//     .selectAll("rect")
//     .data(properties)
//     .join("rect")
//     .attr("x", (d) => x(d.Scheme + " " + d.Parameterset))
//     .attr("y", (d) => y(d.Pk))
//     .attr("height", (d) => y(1) - y(d.Pk))
//     .attr("width", x.bandwidth());

//   svg.call(zoom);

//   svg.append("g").attr("class", "x-axis").call(xAxis);

//   svg.append("g").attr("class", "y-axis").call(yAxis);

//   return svg.node();
// }

// const plot = Plot.plot({
//   x: { label: "Key size (bytes)", type: "log" },
//   y: { padding: 3},
//   marginLeft: 150,
//   width: 1000,
//   height: 9000,
//   color: true,
//   marks: [
//     Plot.barX(properties, {
//       y: (d) => d.Scheme + " " + d.Parameterset,
//       x1: 1,
//       x2: (d) => {
//         console.log(d.Scheme, d.Pk);
//         return d.Pk;
//       },
//       tip: true,
//       width: 1,
//       dy: -2,
//     }),
//     Plot.barX(properties, {
//       y: (d) => d.Scheme + " " + d.Parameterset,
//       x1: 1,
//       x2: (d) => {
//         console.log(d.Scheme, d.Sig);
//         return d.Sig;
//       },
//       tip: true,
//       dy: 2,
//       fill: "red",
//       label: "Signature",
//     }),
//   ],
// });

function dotColor(d) {
  if (d.Classical) { return "blue"; }
  if (d.Broken) {
    return "red";
  }
  if (["Dilithium", "Falcon", "SPHINCS+"].includes(d.Scheme)) {
    return "magenta";
  }
  return "black";
}

function dotSymbol(d) {
  if (["Dilithium", "Falcon", "SPHINCS+"].includes(d.Scheme)) {
    return "star";
  }
  if (d.Classical) {
    return "circle";
  }
  if (d.Broken) {
    return "times";
  }
  return "plus";
}

function dotTitle(d) {
  let str =
    d.Scheme +
    " " +
    d.Parameterset +
    "\npk: " +
    d.Pk.toLocaleString() +
    " B" +
    "\nsig: " +
    d.Sig.toLocaleString() +
    " B";
  if (d.Broken) {
    str += "\n ⚠️ " + d.Broken + "!";
  }
  return str;
}

function updatePlot() {
  const data = sortAndFilterProperties(properties);
  const plot = Plot.plot({
    x: { type: "log", label: "Public key size (bytes)" },
    y: { type: "log", label: "Signature size (bytes)" },
    width: "1000",
    grid: true,
    marks: [
      Plot.dot(data, {
        x: "Pk",
        y: "Sig",
        tip: true,
        title: dotTitle,
        stroke: dotColor,
        symbol: dotSymbol,
        fill: dotColor,
        legend: (d) => d.Category,
      }),
      Plot.crosshair(data, { x: "Pk", y: "Sig" }),
    ],
  });

  document.querySelector("#keySizeChart").replaceChildren(plot);
}
