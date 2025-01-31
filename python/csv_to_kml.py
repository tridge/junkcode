#!/usr/bin/env python3

import csv
import math
import argparse
import xml.etree.ElementTree as ET

def haversine_distance(lat1, lon1, lat2, lon2):
    """
    Calculate the great-circle distance between two points on Earth
    (in decimal degrees). Returns the distance in kilometers.
    """
    # Convert degrees to radians
    lat1_rad, lon1_rad = math.radians(lat1), math.radians(lon1)
    lat2_rad, lon2_rad = math.radians(lat2), math.radians(lon2)

    # Haversine formula
    dlat = lat2_rad - lat1_rad
    dlon = lon2_rad - lon1_rad
    a = math.sin(dlat / 2)**2 + math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(dlon / 2)**2
    c = 2 * math.asin(math.sqrt(a))

    # Radius of the Earth in kilometers (approximately)
    R = 6371.0
    return R * c

def csv_to_kml(csv_file, kml_file, center_lat=None, center_lon=None, max_distance=None):
    """
    Convert the CSV rows into KML placemarks. If center_lat, center_lon, and max_distance
    are provided, then only rows within max_distance kilometers of (center_lat, center_lon)
    are included. Otherwise, all rows are included.
    
    The CSV is expected to have columns:
    Year, MNO, RFNSA ID, Latitude, Longitude, ...
    """

    # Create the KML root element with the standard KML namespace
    kml_ns = "http://www.opengis.net/kml/2.2"
    kml_root = ET.Element("kml", xmlns=kml_ns)
    document = ET.SubElement(kml_root, "Document")

    with open(csv_file, "r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)

        for row in reader:
            lat_str = row.get("Latitude", "").strip()
            lon_str = row.get("Longitude", "").strip()

            # Skip rows without valid coordinates
            if not lat_str or not lon_str:
                continue

            try:
                lat = float(lat_str)
                lon = float(lon_str)
            except ValueError:
                # Skip rows that fail to parse as float
                continue

            # If distance filtering is enabled, check distance
            if center_lat is not None and center_lon is not None and max_distance is not None:
                dist_km = haversine_distance(center_lat, center_lon, lat, lon)
                if dist_km > max_distance:
                    # Skip if beyond the distance threshold
                    continue

            # Create a Placemark element
            placemark = ET.SubElement(document, "Placemark")

            # Use RFNSA ID (or any other field) as the Placemark name
            name_element = ET.SubElement(placemark, "name")
            name_element.text = row.get("RFNSA ID", "Unknown RFNSA ID")

            # Build a description from all columns
            desc_lines = []
            for col_name, value in row.items():
                desc_lines.append(f"{col_name}: {value}")
            desc_text = "\n".join(desc_lines)

            desc_element = ET.SubElement(placemark, "description")
            desc_element.text = desc_text

            # Create Point/coordinates sub-elements
            point = ET.SubElement(placemark, "Point")
            coords = ET.SubElement(point, "coordinates")
            # KML wants longitude,latitude[,altitude]
            coords.text = f"{lon},{lat}"

    # Write the KML document to file
    tree = ET.ElementTree(kml_root)
    tree.write(kml_file, encoding="utf-8", xml_declaration=True)

def main():
    parser = argparse.ArgumentParser(
        description="Convert a CSV file to a KML file, with optional distance filtering."
    )
    parser.add_argument("input_csv", help="Path to the input CSV file.")
    parser.add_argument("output_kml", help="Path to the output KML file.")

    parser.add_argument(
        "--center-lat",
        type=float,
        default=None,
        help="Center latitude for distance filtering."
    )
    parser.add_argument(
        "--center-lon",
        type=float,
        default=None,
        help="Center longitude for distance filtering."
    )
    parser.add_argument(
        "--max-distance",
        type=float,
        default=None,
        help="Maximum distance (in km) from the center point to include in the KML."
    )

    args = parser.parse_args()

    csv_to_kml(
        args.input_csv,
        args.output_kml,
        center_lat=args.center_lat,
        center_lon=args.center_lon,
        max_distance=args.max_distance
    )
    print(f"KML has been written to: {args.output_kml}")

if __name__ == "__main__":
    main()
